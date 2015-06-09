
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
    
    QString text;
    uint n_pages = doc.pages();
    qDebug() << n_pages;
    
    for(uint x=0; x<n_pages; ++x) {
        doc.requestTextPage(x);
        const Okular::Page * pg = doc.page(x);
        QString page = pg->text(NULL);
        qDebug() << page;
        text.append( page );
    }
    

    //return qa.exec();
    //page->search(QString("non-ascii:"), pageRegion, Poppler::Page::FromTop, Poppler::Page::CaseSensitive)
    
    return EXIT_SUCCESS;
}
