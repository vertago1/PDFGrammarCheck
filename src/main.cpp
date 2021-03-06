
//requires packages libboost-regex-dev okular-dev
//requires LanguageTool v2.8

#include <cstdlib>

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <set>
#include <vector>

#include <okular/core/global.h>
#include <okular/core/annotations.h>
#include <okular/core/area.h>
#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/settings_core.h>

#include <QtWidgets/QApplication>
#include <QtGui/QPalette>
#include <QtWidgets/QWidget>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QMimeDatabase>
#include <QtCore/QMimeType>
#include <QtCore/QXmlStreamReader>

#include <boost/xpressive/xpressive.hpp>
#define regex_ns boost::xpressive

// http://localhost:8080/source/xref/okular-14.12.3/ui/pageview.cpp#829
// http://localhost:8080/source/xref/okular-14.12.3/ui/pageview.cpp#869
// http://localhost:8080/source/xref/okular-14.12.3/generators/poppler/generator_pdf.cpp#541
// http://localhost:8080/source/xref/okular-14.12.3/generators/poppler/generator_pdf.cpp#1292

QString parent_path;
std::string parent_path_std;

namespace boost{
    void throw_exception( std::exception const & e ){
        std::cerr <<e.what() <<"\n";
    }
}

inline std::string qstrr2str(const QStringRef& qstr) {
    //return std::string(qstr.toAscii().constData());
    return std::string(qstr.toUtf8().constData());
}

inline std::string qstr2str(const QString& qstr) {
    //return std::string(qstr.toAscii().constData());
    return std::string(qstr.toUtf8().constData());
}

inline QString filter_qstr(const QString& qstr) {
    QString result(qstr);
    result.replace(QString::fromUtf8("\xe2\x80\x9c"),"\"",Qt::CaseSensitive);
    result.replace(QString::fromUtf8("\xe2\x80\x9d"),"\"",Qt::CaseSensitive);
    return result;
}

inline int effective_length(const QString& qstr) {
    int result = qstr.size();
    if(result>0 && qstr.at(result-1)=='-') { --result; }
    return result;
}

inline QString str2qstr(const std::string& str) {
    return QString(str.c_str());
}

inline Okular::RegularAreaRect * find_in_page(const Okular::Page* page, QString text, Okular::RegularAreaRect * rar=NULL){
    Okular::RegularAreaRect * result = NULL;
    Okular::RegularAreaRect * last_found = rar;
    bool retry = rar!=NULL;
    int start = 0, at = 0;
    auto end = text.size();
    while(at!=end && !text.at(at).isLetterOrNumber()){
        ++at;
        start=at;
    }
    while(at!=end){
        while(at!=end && text.at(at)!='?' && text.at(at)!='"'){ ++at; }
        if(at!=end){
            if(at!=start){
                QString search = text.mid(start,at-start);
                Okular::RegularAreaRect * found = page->findText(0,search,(last_found==NULL||last_found->size()==0?Okular::FromTop:Okular::NextResult),Qt::CaseSensitive,last_found);
                
                if(found==NULL){
                    delete result;
                    std::cerr <<"ERR: " <<start <<", " <<at <<", " <<end <<": " <<qstr2str(search) <<"\n";
                    return retry?find_in_page(page,text):NULL;
                } else if(result!=NULL) {
                    result->appendArea(found);
                    delete found;
                } else {
                    result = found;
                    last_found = result;
                }
                ++at;
                start=at;
            } else {
                ++at;
            }
            while(at!=end && !text.at(at).isLetterOrNumber()){
                ++at;
                start=at;
            }
        }
    }
    if(at==end && at!=start){
        if(text.at(at-1)=='-'){
            --at;
        }
        if(at!=start){
            QString search = text.mid(start,at-start);
            Okular::RegularAreaRect * found = page->findText(0,search,(last_found==NULL||last_found->size()==0?Okular::FromTop:Okular::NextResult),Qt::CaseSensitive,last_found);
            
            if(found==NULL){
                std::cerr <<"ERR: " <<start <<", " <<at <<", " <<end <<", " <<(last_found==NULL?0:last_found->size()) <<": " <<qstr2str(search) <<"\n";
                delete result;
                return retry?find_in_page(page,text):NULL;
            } else if(result!=NULL) {
                result->appendArea(found);
                delete found;
            } else {
                result = found;
                last_found = result;
            }
        }
    }
    return result;
}

regex_ns::basic_regex<std::string::iterator> regex_hypen_word = regex_ns::basic_regex<std::string::iterator>::compile("([A-Z]?[a-z]*)-$\n([a-z]+[,.?;]?)|(\n)");

class page_record {
public:
    page_record(const Okular::Page * p) {
        page = p;
        page_text = qstr2str(filter_qstr(p->text(NULL)));
        filter_hyphens();
    }
    
    const Okular::Page* getPage() const { return page; }
    std::string get_text() const { return page_text; }
    const std::vector<size_t> get_lines() const { return lines; }
    const std::vector<std::shared_ptr<Okular::RegularAreaRect> > get_line_rectangles() const { return line_rectangles; }
    size_t get_line_prefix_length(size_t line_no) const {
        if(line_no==0){
            return 0;
        }
        auto itr = hypens.find(line_no-1);
        if(itr==hypens.end()){
            return 0;
        }
        return lines[line_no]-lines[line_no-1]-itr->second-1;
    }
    const std::string get_hypenated_line(size_t line_no) const {
        assert((line_no+1) < lines.size());
        std::string line;
        if(line_no>0){
            auto itr = hypens.find(line_no-1);
            if(itr!=hypens.end()){
                line=std::string(page_text.begin()+lines[line_no-1]+itr->second,page_text.begin()+lines[line_no]-1);
            }
        }
        auto itr = hypens.find(line_no);
        if(itr==hypens.end()){
            if(line_no+2>=lines.size()){
                line.append(std::string(page_text.begin()+lines[line_no],page_text.end()));
            } else {
                line.append(std::string(page_text.begin()+lines[line_no],page_text.begin()+(lines[line_no+1]-1)));
            }
            
        } else {
            line.append(std::string(page_text.begin()+lines[line_no],page_text.begin()+lines[line_no]+itr->second));
            line.append("-");
        }
        return line;
    }
    size_t line_count() const { return lines.size()-1; }
private:
    const Okular::Page * page;
    std::vector<size_t> lines; //starts from zero
    std::vector<std::shared_ptr<Okular::RegularAreaRect> > line_rectangles;
    std::map<size_t,size_t> hypens; //starts from zero
    std::string page_text;
    
    void filter_hyphens(){
        regex_ns::match_results<std::string::iterator> sm;
        auto begin=page_text.begin();
        auto end=page_text.end();
        std::string result;
        size_t line=0;
        size_t col;
        std::shared_ptr<Okular::RegularAreaRect> last_line;
        QString toFind;
        while(regex_ns::regex_search(begin,end, sm, regex_hypen_word)){
            lines.push_back(result.size());
            result.append(sm.prefix());
            if (*(sm.begin()+3)=="\n"){
                toFind = str2qstr(sm.prefix());
                result.append(*sm.begin());
            } else {
                toFind = str2qstr(sm.prefix()+*(sm.begin()+1));
                col=sm.prefix().length()+(*(sm.begin()+1)).length();
                hypens.insert(std::make_pair(line,col));
                result.append(*(sm.begin()+1)+*(sm.begin()+2)+"\n");
            }
            if(toFind.size()>0) {
                Okular::RegularAreaRect* found;
                found = find_in_page(page,toFind,last_line.get());
                if(found==NULL){
                    std::cerr <<"WARNING: unable to find line: " <<line <<") " <<sm.prefix() <<"\n";
                } else {
                    last_line.reset(found);
                }
            }
            line_rectangles.push_back(last_line);
            
            begin = sm.suffix().first;
            //page_text.replace(sm.position()+(begin-page_text.begin()),(*sm.begin()).length(),*(sm.begin()+1)+*(sm.begin()+2)+"\n");
            //begin = sm.prefix().second;
            ++line;
        }
        lines.push_back(result.size());
        result.append(begin,end);
        page_text=result;
    }
};

std::string check_spelling_and_grammar(std::string text){
    int len = 4096;
    char name[len];
    std::sprintf(name,"/tmp/");
    std::string temp_file_name = std::string(tmpnam(name))+"";
    std::sprintf(name,"/tmp/");
    std::string temp_file_name2 = std::string(tmpnam(name))+"";
    std::ofstream outfile(temp_file_name);
    outfile << text;
    outfile.close();
    
    int ret_val = system((std::string("java -jar $HOME/LanguageTool/languagetool-commandline.jar --api -l en-US ")+std::string(temp_file_name)+std::string(" > ")+std::string(temp_file_name2)).c_str());
    if(ret_val!=0){
        exit(ret_val);
    }
    
    std::ifstream ifs(temp_file_name2);
    
    std::string result((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
    ifs.close();
    
    std::remove(temp_file_name.c_str());
    std::remove(temp_file_name2.c_str());
    
    return result;
}

void write_total_text(std::string text){
    std::ofstream outfile(parent_path_std+"pdf.txt");
    outfile << text;
    outfile.close();
}

void write_sample_result(std::string result){
    std::ofstream outfile(parent_path_std+"checked.xml");
    outfile << result;
    outfile.close();
}

std::string get_sample_result(){
    std::ifstream ifs(parent_path_std+"checked.xml");
    std::string result((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
    ifs.close();
    return result;
}

int main(int argc, char **argv) {
    parent_path = QString(argv[0]);
    parent_path_std = std::string(argv[0]);
    QApplication qa(argc, argv);
    QWidget window;
    
    window.resize(250, 150);
    window.setWindowTitle("Simple example");
    
    Okular::SettingsCore::instance("LanguageTool");
    QString path;
    if(argc>1){
        path = QString(argv[argc-1]);
    } else {
        path = QString(parent_path+"sample.pdf");
    }
    
    QString out_path(path);
    if(out_path.endsWith(".pdf",Qt::CaseInsensitive)){
        out_path = out_path.left(out_path.size()-4);
    }
    out_path+=".chkd.pdf";
    const QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(path);
    QString pswd;
    Okular::Document doc(&window);
    doc.openDocument(path,QUrl(),mime,pswd);
    
    std::string text;
    std::vector<page_record *> pages;
    std::map<int,page_record*> line_no_page_index;
    
    uint n_pages = doc.pages();
    qDebug() << n_pages;
    
    int line_no=0;
    for(uint x=0; x<n_pages; ++x) {
        doc.requestTextPage(x);
        const Okular::Page * pg = doc.page(x);
        
        page_record* page_rec = new page_record(pg);
        
        pages.push_back(page_rec);
        line_no_page_index.insert(std::make_pair(line_no,page_rec));
        text+=page_rec->get_text();
        line_no+=page_rec->line_count();
    }
    
//    for(uint x=0; x<n_pages; ++x) {
//        std::string text = pages[x]->get_text();
//        std::cout <<") " <<text <<"\n";
//        std::cout <<") " <<text.size() <<"\n";
//        for(uint y=0; y<pages[x]->line_count(); ++y){
//            std::cout <<"= " <<*(pages[x]->get_lines().begin()+y) <<"\n";
//            std::cout <<"+ " <<pages[x]->get_hypenated_line(y) <<"\n";
//            std::cout <<"= " <<(pages[x]->get_lines()[y+1]-1)-(pages[x]->get_lines()[y]) <<"\n";
//            
//            std::cout <<"- " <<std::string(text.begin()+pages[x]->get_lines()[y],text.begin()+pages[x]->get_lines()[y+1]-1) <<"\n";
//        }
//    }
//    exit(0);
    
    write_total_text(text);
    std::string result = check_spelling_and_grammar(text);
    //write_sample_result(result);
    //std::string result = get_sample_result();
    
    //https://github.com/languagetool-org/languagetool/blob/master/languagetool-core/src/main/resources/org/languagetool/resource/api-output.dtd
    enum : int {
        //The line in which the error starts. Counting starts at 0.
        flag_fromy               = 0x00000001,
        //The column in which the error starts. Counting starts at 0.
        flag_fromx               = 0x00000002,
        //The line in which the error ends:
        flag_toy                 = 0x00000004,
        //The column in which the error ends:
        flag_tox                 = 0x00000008,
        //An internal ID that refers to the error rule:
        flag_ruleid              = 0x00000010,
        //The message describing the error that will be displayed to the user.
        flag_msg                 = 0x00000020,
        //One or more suggestions to fix the error. If there is more than one
        //suggestion, the strings are separated by a "#" character:
        flag_replacements        = 0x00000040,
        //The context or sentence in which the error occurs.
        flag_context             = 0x00000080,
        //The position of the start of the error in the 'context'
        //attribute. Counting starts at 0.
        flag_contextoffset       = 0x00000100,
        //The position of the start of the error in the input text.
        //Counting starts at 0. (added in LanguageTool 1.9)
        flag_offset              = 0x00000200,
        //The length of the error in the input text.
        flag_errorlength         = 0x00000400,
        //The category of the match, if any (added in LanguageTool 1.9).
        flag_category            = 0x00000800,
        //Localization Quality Issue Type, according to Internationalization
        //Tag Set (ITS) Version 2.0
        flag_locqualityissuetype = 0x00001000,
    };
    
    std::map<std::string,int> attr_lookup({
        std::make_pair("fromy",               flag_fromy),
        std::make_pair("fromx",               flag_fromx),
        std::make_pair("toy",                 flag_toy),
        std::make_pair("tox",                 flag_tox),
        std::make_pair("ruleId",              flag_ruleid),
        std::make_pair("msg",                 flag_msg),
        std::make_pair("replacements",        flag_replacements),
        std::make_pair("context",             flag_context),
        std::make_pair("contextoffset",       flag_contextoffset),
        std::make_pair("offset",              flag_offset),
        std::make_pair("errorlength",         flag_errorlength),
        std::make_pair("category",            flag_category),
        std::make_pair("locqualityissuetype", flag_locqualityissuetype),
    });
    
    QString author("LanguageTool 2.8");
    
    QXmlStreamReader xml_parser(QString(result.c_str()));
    size_t last_fromy=0, last_fromx=0, last_tox=0, last_toy=0;
    Okular::RegularAreaRect* last_found = NULL;
    
    while (!xml_parser.atEnd()) {
        QXmlStreamReader::TokenType type=xml_parser.readNext();
        if(type==QXmlStreamReader::TokenType::StartElement && xml_parser.name()=="error"){
            size_t fromy=0, fromx=0, tox=0, toy=0, contextoffset=0, offset=0, errorlength=0;
            std::string ruleid, msg, replacements, context, category, locqualityissuetype;
            uint32_t flags=0;
            for(const auto attribute : xml_parser.attributes()){
                //qDebug() << attribute.name();
                auto itr = attr_lookup.find(qstrr2str(attribute.name()));
                if(itr!=attr_lookup.end()){
                    switch(itr->second){
                        case flag_fromy:
                            fromy=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_fromy;
                            break;
                        case flag_fromx:
                            fromx=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_fromx;
                            break;
                        case flag_toy:
                            toy=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_toy;
                            break;
                        case flag_tox:
                            tox=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_tox;
                            break;
                        case flag_ruleid:
                            ruleid=qstrr2str(attribute.value());
                            flags|=flag_ruleid;
                            break;
                        case flag_msg:
                            msg=qstrr2str(attribute.value());
                            flags|=flag_msg;
                            break;
                        case flag_replacements:
                            replacements=qstrr2str(attribute.value());
                            flags|=flag_replacements;
                            break;
                        case flag_context:
                            context=qstrr2str(attribute.value());
                            flags|=flag_context;
                            break;
                        case flag_contextoffset:
                            contextoffset=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_contextoffset;
                            break;
                        case flag_offset:
                            offset=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_offset;
                            break;
                        case flag_errorlength:
                            errorlength=std::stoi(qstrr2str(attribute.value()));
                            flags|=flag_errorlength;
                            break;
                        case flag_category: //std::make_pair("category",            12),
                            category=qstrr2str(attribute.value());
                            flags|=flag_category;
                            break;
                        case flag_locqualityissuetype: //std::make_pair("locqualityissuetype", 13),
                            locqualityissuetype=qstrr2str(attribute.value());
                            flags|=flag_locqualityissuetype;
                            break;
                    }
                } else {
                    //std::cerr <<"ERROR couldn't find: " <<qstrr2str(attribute.name()) <<"\n";
                }
            } //done parsing xml tag
            if(((~flags)&(flag_fromy|flag_fromx|flag_toy|flag_tox))==0){
                
                auto itr_page_rec = line_no_page_index.upper_bound(fromy);
                if(itr_page_rec==line_no_page_index.begin()) {
                    std::cerr <<"Warning: failed to lookup page by line number!\n";
                    std::cerr <<fromy <<" " <<itr_page_rec->first <<"\n";
                    break;
                }
                --itr_page_rec;
                int local_line_no = fromy - itr_page_rec->first;
                page_record * page_rec = itr_page_rec->second;
                auto page = page_rec->getPage();
                
                Okular::RegularAreaRect* found = NULL;
                bool use_cache=last_fromy==fromy && last_fromx==fromx && last_toy==toy && last_tox==tox;
                
                last_fromy=fromy;
                last_fromx=fromx;
                last_toy=toy;
                last_tox=tox;
                
                if(use_cache){
                    found = last_found;
                    if(found==NULL){
                        continue;
                    }
                } else {
                    QString line;
                    if (local_line_no >= page_rec->line_count()) {
                      line = ""; //TODO handle cross page hyphens
                    } else {
                      line = str2qstr(page_rec->get_hypenated_line(local_line_no));
                    }

                    size_t prefix_length = page_rec->get_line_prefix_length(local_line_no);
                    std::cout <<"fromx: " << fromx <<" prefix_length: " <<prefix_length <<"\n";
                    fromx += prefix_length;
                    bool wrap=true;

                    int e_length = effective_length(line);

                    if(((int)fromx)>e_length){
                        fromx-=e_length;
                        fromy+=1;
                        local_line_no+=1;
                        std::cout <<"fromx: " << fromx <<" old line: " <<qstr2str(line) <<"\n";
                        line = str2qstr(page_rec->get_hypenated_line(local_line_no));
                        prefix_length = page_rec->get_line_prefix_length(local_line_no);
                        std::cout <<"new line:  " <<qstr2str(line) <<"\n";
                    }

                    if(fromy==toy){
                        tox += prefix_length;
                        if(((int)tox)>e_length){
                            tox-=e_length;
                            toy+=1;
                        } else {
                            assert(tox>fromx);
                            line = line.mid(fromx,tox-fromx);
                        }
                        wrap=false;
                    }
                    if(fromy!=toy){
                        line = line.right(line.size()-fromx);
                    }

                    if(local_line_no>0){
                        std::shared_ptr<Okular::RegularAreaRect> last_line = page_rec->get_line_rectangles()[local_line_no-1];
                        found = last_line.get();
                    }
                    found = find_in_page(page,line,found);
                    
                    if(last_found!=NULL){
                        delete last_found;
                    }
                    last_found=found;

                    if(found==NULL || found->size()<1){
                        std::cout <<"-- " <<qstr2str(line) <<": " <<context <<"\n";
                        continue;
                    } else {
                        Okular::RegularAreaRect* last = found;
                        while(fromy<toy){
                            ++fromy;
                            ++local_line_no;
                            Okular::RegularAreaRect* also = NULL;
                            line = str2qstr(page_rec->get_hypenated_line(local_line_no));
                            if(fromy==toy){
                                if(wrap){
                                    tox += page_rec->get_line_prefix_length(local_line_no);
                                    e_length = effective_length(line);
                                    if(((int)tox)>e_length){
                                        tox-=e_length;
                                        toy+=1;
                                    } else {
                                        line = line.left(tox);
                                    }
                                    wrap=false;
                                } else {
                                    line = line.left(tox);
                                }
                            }
                            also=find_in_page(page,line,last);
                            if(last!=NULL && last!=found) { delete last; }
                            last=also;
                            if(also==NULL){
                                break;
                            } else {
                                found->appendArea(also);
                            }
                        }
                        if(last!=NULL && last!=found) { delete last; }
                        std::cout <<"++ " <<qstr2str(line) <<": " <<context <<"\n";
                    }
                }
                
                //std::cout <<fromy <<" " <<itr_page_rec->first <<" " <<local_line_no <<" " <<page_rec->line_count() <<"\n";
                //std::cout <<"== " <<page_rec->get_hypenated_line(local_line_no) <<"\n";
                //std::cout <<"-- " <<context <<"\n";
                
                Okular::HighlightAnnotation * annotation = new Okular::HighlightAnnotation;
                
                //Okular::NormalizedRect bounds(found->front());
                for ( const Okular::NormalizedRect & r : *found ) {
                    //bounds=bounds|r;
                    Okular::HighlightAnnotation::Quad q;
                    q.setCapStart( false );
                    q.setCapEnd( false );
                    q.setFeather( 1.0 );
                    q.setPoint( Okular::NormalizedPoint( r.left, r.bottom ), 0 );
                    q.setPoint( Okular::NormalizedPoint( r.right, r.bottom ), 1 );
                    q.setPoint( Okular::NormalizedPoint( r.right, r.top ), 2 );
                    q.setPoint( Okular::NormalizedPoint( r.left, r.top ), 3 );
                    annotation->highlightQuads().append( q );
                    std::cout <<r.left <<" " <<r.right <<" " <<r.top <<" " <<r.bottom <<"\n";
                }
                if(flag_ruleid && ruleid=="MORFOLOGIK_RULE_EN_US"){
                    annotation->setHighlightType(Okular::HighlightAnnotation::Squiggly);
                    annotation->style().setColor(QColor(255,128,128));
                    annotation->style().setOpacity(1.0);
                } else {
                    annotation->setHighlightType(Okular::HighlightAnnotation::Highlight);
                    annotation->style().setColor(QColor(128,128,255));
                    annotation->style().setOpacity(1.0);
                }
                
                
                QString contents;
                if((flags & flag_msg)!=0){
                    contents.append(QString::fromUtf8(msg.c_str()));
                }
                if((flags & flag_replacements)!=0){
                    if(contents.length()>0){
                        contents.append("\n");
                    }
                    contents.append("Suggestions: ");
                    auto itr = replacements.begin();
                    auto start = itr;
                    while(itr!=replacements.end()){
                        if(*itr=='#'){
                            contents.append(QString::fromUtf8(std::string(start,itr).c_str()));
                            contents.append(", ");
                            start=itr+1;
                        }
                        ++itr;
                    }
                    contents.append(QString::fromUtf8(std::string(start,itr).c_str()));
                }
                annotation->setAuthor(author);
                annotation->setCreationDate( QDateTime::currentDateTime() );
                annotation->setModificationDate( QDateTime::currentDateTime() );
                annotation->setContents(filter_qstr(contents));
                doc.addPageAnnotation(page->number(),annotation);
            }
        }
    }
    
    doc.saveChanges(out_path);
    
    for(page_record * rec : pages) { delete rec; }
    pages.clear();
    line_no_page_index.clear();
    
    //return qa.exec();
    //page->search(QString("non-ascii:"), pageRegion, Poppler::Page::FromTop, Poppler::Page::CaseSensitive)
    
    return EXIT_SUCCESS;
}
