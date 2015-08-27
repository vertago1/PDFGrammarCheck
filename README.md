# PDFGrammarCheck

A C++11 program that leverages Okular's PDF library and LanguageTool to perform
a spelling and grammar check on PDF files. The results are saved as a new pdf
file with annotations containing the suggestions from LanguageTool. A before and
after demonstration can be seen with `sample.pdf` and `sample.chkd.pdf`.

This software is in a alpha/beta quality state.

Okular was chosen to be used for handling the PDF functionality because it does
a good job of selecting the text from a PDF in the correct word order. I tried
several other options prior, none of which worked as well as Okular's core
library.

## Usage

Once compiled simply provide the path to the pdf file you wish to check:

    ./languagetool_pdf sample.pdf

The resulting file will be saved to sample.chck.pdf with the suggestions
annotated.

## Requirements

PDFGrammarCheck was written and tested on both Kubuntu 15.04 and 15.10a2 with
LanguageTool 2.8.

Besides `java`, `build-essential`, and `cmake`, the following additional packages are
required:

* `libboost-regex-dev` for `<boost/xpressive/xpressive.hpp>` a fast header-only
implementation of regular expressions
* `okular-dev` for the okular core library headers
* LanguageTool should be extracted to ~/LanguageTool

To find the proper libaries to link to support okular cmake will check for KDE4
and QT4. These are be present by default on Kubuntu 15.04.

## Building PDFGrammarCheck

After obtaining all the prequisites, here is an example of how to build
PDFGrammarCheck on Kubuntu 15.04:

    git clone https://github.com/vertago1/PDFGrammarCheck.git
    mkdir PDFGrammarCheck/build
    cd PDFGrammarCheck/build/
    cmake ../
    make
    cd ../

The PDFGrammarCheck binary can then be run by:

    ./build/src/langtool_pdf sample.pdf

The result will show up as `sample.chkd.pdf`.

## Troubleshooting

### Error: Unable to access jarfile ~/LanguageTool/languagetool-commandline.jar

Make sure to have put your LanguageTool installation or linked to it at
`$HOME/LanguageTool`.