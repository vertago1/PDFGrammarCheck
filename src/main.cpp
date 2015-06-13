
//requires libboost-regex-dev okular-dev

#include <cstdlib>

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <set>
#include <vector>

#include <QtGui/QApplication>
#include <QtGui/QPalette>
#include <QtGui/QWidget>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QXmlStreamReader>

#include <okular/core/global.h>
#include <okular/core/area.h>
#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/settings_core.h>

//#include "okular-14.12.3/core/global.h"
//#include "okular-14.12.3/core/area.h"
//#include "okular-14.12.3/core/document.h"
//#include "okular-14.12.3/core/generator.h"
//#include "okular-14.12.3/core/page.h"
//#include "okular-14.12.3/obj-x86_64-linux-gnu/settings_core.h"

//#include <poppler/qt4/poppler-qt4.h>

//#include "poppler/PDFDoc.h"
//#include "poppler/ErrorCodes.h"
//#include "poppler/TextOutputDev.h"

#define RUN_PATH "/home/vertago1/Documents/unison/SourceCode/LanguageToolPDFCPP/"

#include <boost/xpressive/xpressive.hpp>
#define regex_ns boost::xpressive

// http://localhost:8080/source/xref/okular-14.12.3/ui/pageview.cpp#829
// http://localhost:8080/source/xref/okular-14.12.3/ui/pageview.cpp#869
// http://localhost:8080/source/xref/okular-14.12.3/generators/poppler/generator_pdf.cpp#541
// http://localhost:8080/source/xref/okular-14.12.3/generators/poppler/generator_pdf.cpp#1292

namespace boost{
    void throw_exception( std::exception const & e ){
        std::cerr <<e.what() <<"\n";
    }
}

inline std::string qstr2str(const QString& qstr) {
    return std::string(qstr.toAscii().constData());
}

inline QString str2qstr(const std::string& str) {
    return QString(str.c_str());
}

regex_ns::basic_regex<std::string::iterator> regex_hypen_word = regex_ns::basic_regex<std::string::iterator>::compile("([A-Z]?[a-z]*)-$\n([a-z]+[,.?]?) ?|[^-](\n)");

class page_record {
public:
    page_record(std::string text) {
        page_text = text;
        lines = std::count<std::string::iterator,char>(page_text.begin(),page_text.end(),'\n');
    }
    
    void filter_hyphens(){
        regex_ns::match_results<std::string::iterator> sm;
        auto begin=page_text.begin();
        auto end=page_text.end();
        std::string result;
        size_t line=0;
        size_t col;
        while(regex_ns::regex_search(begin,end, sm, regex_hypen_word)){
            ++line;
            result.append(sm.prefix());
            if (*(sm.begin()+3)=="\n"){
                
            } else {
                col=sm.prefix().length()+(*(sm.begin()+1)).length()+1;
                hypens.insert(std::make_pair(line,col));
                result.append(*(sm.begin()+1)+*(sm.begin()+2)+"\n");
            }
            begin = sm.suffix().first;
            //page_text.replace(sm.position()+(begin-page_text.begin()),(*sm.begin()).length(),*(sm.begin()+1)+*(sm.begin()+2)+"\n");
            //begin = sm.prefix().second;
        }
        result.append(begin,end);
        page_text=result;
    }
    
    std::string get_text() const { return page_text; }
    int get_lines() const { return lines; }
private:
    std::map<size_t,size_t> hypens;
    std::string page_text;
    int lines;
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
    
    int ret_val = system((std::string("java -jar " RUN_PATH "LanguageTool-2.8/languagetool-commandline.jar --api -l en-US ")+std::string(temp_file_name)+std::string(" > ")+std::string(temp_file_name2)).c_str());
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
    std::ofstream outfile(RUN_PATH "pdf.txt");
    outfile << text;
    outfile.close();
}

void write_sample_result(std::string result){
    std::ofstream outfile(RUN_PATH "checked.xml");
    outfile << result;
    outfile.close();
}

std::string get_sample_result(){
    std::ifstream ifs(RUN_PATH "checked.xml");
    std::string result((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
    ifs.close();
    return result;
}

int main(int argc, char **argv) {
    QApplication qa(argc, argv);
    QWidget window;

    window.resize(250, 150);
    window.setWindowTitle("Simple example");
    
    Okular::SettingsCore::instance("LanguageTool");
    QString path(RUN_PATH "sample.pdf");
    const KMimeType::Ptr mime = KMimeType::findByPath(path);
    QString pswd;
    Okular::Document doc(&window);
    doc.openDocument(path,KUrl(),mime,pswd);
    
    std::string text;
    std::vector<page_record *> pages;
    std::map<int,page_record*> line_no_page_index;
    
    uint n_pages = doc.pages();
    qDebug() << n_pages;
    
    int line_no=0;
    for(uint x=0; x<n_pages; ++x) {
        doc.requestTextPage(x);
        const Okular::Page * pg = doc.page(x);
        QString page = pg->text(NULL);
        
        page_record* page_rec = new page_record(std::string(page.toAscii().constData()));
        page_rec->filter_hyphens();
        
        pages.push_back(page_rec);
        line_no_page_index.insert(std::make_pair(line_no,page_rec));
        text+=page_rec->get_text();
        line_no+=page_rec->get_lines();
    }
    
    //write_total_text(text);
    //std::string result = check_spelling_and_grammar(text);
    //write_sample_result(result);
    std::string result = get_sample_result();
    
    enum : int {
        flag_fromy               = 0x00000001,
        flag_fromx               = 0x00000002,
        flag_toy                 = 0x00000004,
        flag_tox                 = 0x00000008,
        flag_ruleid              = 0x00000010,
        flag_msg                 = 0x00000020,
        flag_replacements        = 0x00000040,
        flag_context             = 0x00000080,
        flag_contextoffset       = 0x00000100,
        flag_offset              = 0x00000200,
        flag_errorlength         = 0x00000400,
        flag_category            = 0x00000800,
        flag_locqualityissuetype = 0x00001000,
    };
    
    std::map<std::string,int> attr_lookup({
        std::make_pair("fromy",               flag_fromy),
        std::make_pair("fromx",               flag_fromx),
        std::make_pair("toy",                 flag_toy),
        std::make_pair("tox",                 flag_tox),
        std::make_pair("ruleid",              flag_ruleid),
        std::make_pair("msg",                 flag_msg),
        std::make_pair("replacements",        flag_replacements),
        std::make_pair("context",             flag_context),
        std::make_pair("contextoffset",       flag_contextoffset),
        std::make_pair("offset",              flag_offset),
        std::make_pair("errorlength",         flag_errorlength),
        std::make_pair("category",            flag_category),
        std::make_pair("locqualityissuetype", flag_locqualityissuetype),
    });
    
    QXmlStreamReader xml_parser(QString(result.c_str()));
    while (!xml_parser.atEnd()) {
        QXmlStreamReader::TokenType type=xml_parser.readNext();
        if(type==QXmlStreamReader::TokenType::StartElement && xml_parser.name()=="error"){
            size_t fromy, fromx, tox, toy, contextoffset, offset, errorlength;
            std::string ruleid, msg, replacements, context, category, locqualityissuetype;
            uint32_t flags=0;
            for(const auto attribute : xml_parser.attributes()){
                qDebug() << attribute.name();
                auto itr = attr_lookup.find(qstr2str(*attribute.name().string()));
                if(itr!=attr_lookup.end()){
                    switch(itr->second){
                        case flag_fromy:
                            fromy=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_fromy;
                            break;
                        case flag_fromx:
                            fromx=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_fromx;
                            break;
                        case flag_toy:
                            toy=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_toy;
                            break;
                        case flag_tox:
                            tox=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_tox;
                            break;
                        case flag_ruleid:
                            ruleid=qstr2str(*attribute.value().string());
                            flags|=flag_ruleid;
                            break;
                        case flag_msg:
                            msg=qstr2str(*attribute.value().string());
                            flags|=flag_msg;
                            break;
                        case flag_replacements:
                            replacements=qstr2str(*attribute.value().string());
                            flags|=flag_replacements;
                            break;
                        case flag_context:
                            context=qstr2str(*attribute.value().string());
                            flags|=flag_context;
                            break;
                        case flag_contextoffset:
                            contextoffset=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_contextoffset;
                            break;
                        case flag_offset:
                            offset=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_offset;
                            break;
                        case flag_errorlength:
                            errorlength=std::stoi(qstr2str(*attribute.value().string()));
                            flags|=flag_errorlength;
                            break;
                        case flag_category: //std::make_pair("category",            12),
                            category=qstr2str(*attribute.value().string());
                            flags|=flag_category;
                            break;
                        case flag_locqualityissuetype: //std::make_pair("locqualityissuetype", 13),
                            locqualityissuetype=qstr2str(*attribute.value().string());
                            flags|=flag_locqualityissuetype;
                            break;
                    }
                }
            }
        }
    }
    

    //return qa.exec();
    //page->search(QString("non-ascii:"), pageRegion, Poppler::Page::FromTop, Poppler::Page::CaseSensitive)
    
    return EXIT_SUCCESS;
}
