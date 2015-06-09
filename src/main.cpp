
#include <cstdlib>

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
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

regex_ns::basic_regex<std::string::iterator> regex_hypen_word = regex_ns::basic_regex<std::string::iterator>::compile("([A-Z]?[a-z]*)-$\n([a-z]+[,.?]?) ?");

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
        while(regex_ns::regex_search(begin,end, sm, regex_hypen_word)){
            result.append(sm.prefix());
            result.append(*(sm.begin()+1)+*(sm.begin()+2)+"\n");
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
    
    QXmlStreamReader xml_parser(QString(result.c_str()));
    while (!xml_parser.atEnd()) {
        QXmlStreamReader::TokenType type=xml_parser.readNext();
        if(type==QXmlStreamReader::TokenType::StartElement && xml_parser.name()=="error"){
            qDebug() << xml_parser.attributes().constBegin()->name();
        }
        
    }
    

    //return qa.exec();
    //page->search(QString("non-ascii:"), pageRegion, Poppler::Page::FromTop, Poppler::Page::CaseSensitive)
    
    return EXIT_SUCCESS;
}
