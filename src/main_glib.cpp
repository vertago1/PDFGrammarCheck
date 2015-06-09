
#include <cstdlib>

#include <fstream>
#include <iostream>
#include <sstream>

#include <map>
#include <vector>

#include <glib.h>
#include <glib/poppler.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include <glib/poppler-annot.h>

//#include "poppler/PDFDoc.h"
//#include "poppler/ErrorCodes.h"
//#include "poppler/TextOutputDev.h"

#define RUN_PATH "/home/vertago1/Documents/unison/SourceCode/LanguageToolPDFCPP/"

#include <boost/xpressive/xpressive.hpp>

#define regex_ns boost::xpressive

//bool filter_out(char c) { return c==(char)(0x80)||c==(char)(0x93); }
bool filter_out(char c) { return (c&0x80)!=0; }
void filter_string(std::string&line){
    std::remove_if(line.begin(),line.end(),filter_out);
}

class page_record {
public:
    page_record(PopplerPage* p) : page(p) {
        page_text = std::string(poppler_page_get_text(p))+"\n";
        filter_string(page_text);
        lines = std::count<std::string::iterator,char>(page_text.begin(),page_text.end(),'\n');
    }
    
    PopplerPage* get_page() { return page; }
    std::string get_text() const { return page_text; }
    int get_lines() const { return lines; }
private:
    PopplerPage* page;
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
    
    system((std::string("java -jar " RUN_PATH "LanguageTool-2.8/languagetool-commandline.jar -l en-US ")+std::string(temp_file_name)+std::string(" > ")+std::string(temp_file_name2)).c_str());
    
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
    std::ofstream outfile(RUN_PATH "text.txt");
    outfile << result;
    outfile.close();
}

std::string get_sample_result(){
    std::ifstream ifs(RUN_PATH "text.txt");
    std::string result((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
    ifs.close();
    return result;
}

bool get_next_line(std::string::iterator * begin,std::string::iterator * end,std::string::iterator stop,std::string& line){
    while(*end!=stop&&*(*end)!='\n') { ++(*end); }
    line = std::string(*begin,*end);
    if(*end==stop){
        *begin=*end;
        return false;
    } else {
        *begin=*end+1;
        *end=*begin;
        return *end!=stop;
    }
}

int match_until(std::string text,std::string& highlight,int line_no,int column_no){
    int at_line=1, at_col=1;
    int count=0;
    auto itr = text.begin();
    
    int skip=0;
    
    while(itr+highlight.size()!=text.end()&& (at_line<line_no || (at_line==line_no && at_col < column_no))){
        if(skip<1){
            if(text.compare(itr-text.begin(),highlight.size(),highlight)==0){
                ++count;
                skip+=highlight.size();
            }
        } else {
            --skip;
        }
        if(*itr=='\n'){
            ++at_line;
            at_col=1;
        } else {
            ++at_col;
        }
        ++itr;
    }
    if(at_line==line_no && at_col==column_no && text.compare(itr-text.begin(),highlight.size(),highlight)==0){
        //std::cout<<"CHECK!!!\n";
        return count;
    } else if(itr+highlight.size()!=text.end()) {
        highlight = std::string(itr,itr+highlight.size());
        //std::cerr<<"FAIL!!! line=" <<at_line <<'-' <<line_no <<" column=" <<at_col <<'-' <<column_no <<" \"" << <<"\"\n";
        return match_until(text,highlight,line_no,column_no);
    } else {
        return -1;
    }
}

inline void
pgd_annots_set_poppler_quad_from_rectangle(PopplerQuadrilateral *quad,
        PopplerRectangle *rect) {
    quad->p1.x = rect->x1;
    quad->p1.y = rect->y1;
    quad->p2.x = rect->x2;
    quad->p2.y = rect->y1;
    quad->p3.x = rect->x1;
    quad->p3.y = rect->y2;
    quad->p4.x = rect->x2;
    quad->p4.y = rect->y2;
}

GArray *
pgd_annots_create_quads_array_for_rectangle(PopplerRectangle *rect) {
    GArray *quads_array;
    PopplerQuadrilateral *quad;

    quads_array = g_array_sized_new(FALSE, FALSE,
            sizeof (PopplerQuadrilateral),
            1);
    g_array_set_size(quads_array, 1);

    quad = &g_array_index(quads_array, PopplerQuadrilateral, 0);
    pgd_annots_set_poppler_quad_from_rectangle(quad, rect);

    return quads_array;
}

int main() {
    //return process_input(std::cin);
    PopplerDocument* doc = poppler_document_new_from_file("file://" RUN_PATH "sample.pdf",NULL,NULL);
    PopplerColor * highlighter = poppler_color_new();
    highlighter->red=0xffff;
    highlighter->green=0xA000;
    highlighter->blue=0xA000;
        
    int n_pages = poppler_document_get_n_pages(doc);;
    
    std::cout<<"Pages: " <<n_pages <<'\n';
    std::cout.flush();
    
    std::string text;
    std::vector<page_record *> pages;
    
    std::map<int,page_record*> line_no_page_index;
    
    int line_no=0;
    for(int x=0; x<n_pages; ++x){
        PopplerPage* page = poppler_document_get_page(doc,x);
        page_record * page_rec = new page_record(page);
        pages.push_back(page_rec);
        line_no_page_index.insert(std::make_pair(line_no,page_rec));
        text+=page_rec->get_text();
        line_no+=page_rec->get_lines();
        
    }
    
    //write_total_text(text);
    //std::string result = check_spelling_and_grammar(text);
    //write_sample_result(result);
    std::string result = get_sample_result();
    
    const std::string message_prefix("Message: ");
    const std::string suggestion_prefix("Suggestion: ");
        
    std::string::iterator begin=result.begin(), end = result.begin();
    //672.) Line 1539, column 25, Rule ID: MORFOLOGIK_RULE_EN_US
    regex_ns::basic_regex<std::string::iterator> regex_header = regex_ns::basic_regex<std::string::iterator>::compile("^[0-9]+\\.\\) Line ([0-9]+), column ([0-9]+), Rule ID: .*$");
    std::string line;
    while(begin!=result.end() && get_next_line(&begin,&end,result.end(),line)){
        regex_ns::match_results<std::string::iterator> sm;
        if(regex_ns::regex_match(line.begin(),line.end(),sm,regex_header)){
            int line_no = std::stoi(*(sm.begin()+1));
            int column_no = std::stoi(*(sm.begin()+2));
            if(!get_next_line(&begin,&end,result.end(),line)){ std::cerr << "Warning: truncated!\n"; break;}
            std::string message(""), suggestion_text("");
            if(line.size()>message_prefix.size() && line.compare(0, message_prefix.size(), message_prefix)==0){
                message=std::string(line.begin()+message_prefix.size(),line.end());
                if(!get_next_line(&begin,&end,result.end(),line)){ std::cerr << "Warning: truncated!\n"; break;}
            }
            if(line.size()>suggestion_prefix.size() && line.compare(0, suggestion_prefix.size(), suggestion_prefix)==0){
                //suggestion_text=std::string(line.begin()+suggestion_prefix.size(),line.end());
                suggestion_text=line+"";
                if(!get_next_line(&begin,&end,result.end(),line)){ std::cerr << "Warning: truncated!\n"; break;}
            }
            std::string line_text = line+"";
            if(!get_next_line(&begin,&end,result.end(),line)){ std::cerr << "Warning: truncated!\n"; break;}
            std::string highlight = line+"";
            size_t start = 0;
            size_t end = (line_text.size()<highlight.size()) ? line_text.size() : highlight.size();
            while(start<end && highlight[end-1]!='^') { --end; }
            while(start<end && highlight[start]!='^') { ++start; }
            std::string highlighted = (line_text.size()>end) ? std::string(line_text.begin()+start,line_text.begin()+end) : std::string("");
            
            //std::cout <<line_no <<", " <<column_no <<", \"" <<message
            //        <<"\", \"" <<suggestion_text <<"\", \"" <<line_text <<"\", \"" <<highlight<<"\"\n";
            
            assert(line_no>0);
            auto itr_page_rec = line_no_page_index.upper_bound(line_no);
            if(itr_page_rec==line_no_page_index.begin()) {
                std::cerr <<"Warning: failed to lookup page by line number!\n";
                break;
            }
            --itr_page_rec;
            int local_line_no = line_no - itr_page_rec->first;
            page_record * page_rec = itr_page_rec->second;
            
            int index = match_until(page_rec->get_text(),highlighted,local_line_no,column_no);
            if(index<0){
                std::cerr <<"Warning: unable to find text! Probably spans pages.\n";
                continue;
            }
            
            auto list = poppler_page_find_text_with_options(page_rec->get_page(),highlighted.c_str(),POPPLER_FIND_CASE_SENSITIVE);
            int len = g_list_length(list);
            if(index>=len){
                std::cerr <<"Warning: index mismatch with find text (" <<index <<"," <<len <<"): " <<highlighted <<"\n";    
                std::cout <<line_no <<", " <<column_no <<", \"" <<message
                        <<"\", \"" <<suggestion_text <<"\", \"" <<line_text <<"\", \"" <<highlighted<<"\"\n";
                std::cout <<line_text <<"\n" <<highlight <<"\n";
                continue;
            }
            PopplerRectangle * rect = (PopplerRectangle *)g_list_nth_data(list,index);
            PopplerAnnot *annot = poppler_annot_text_markup_new_highlight(doc,rect,pgd_annots_create_quads_array_for_rectangle(rect));
            poppler_annot_set_color(annot,highlighter);
            
            poppler_page_add_annot(page_rec->get_page(),annot);
            poppler_annot_set_contents(annot,(message+"\n"+suggestion_text).c_str());
            g_list_free(list);
        }
        
        begin=end+1;
        end=begin;
    }
            
    
    poppler_document_save(doc,"file://" RUN_PATH "sample.new.pdf",NULL);
    
    free(doc);
    return EXIT_SUCCESS;
}
