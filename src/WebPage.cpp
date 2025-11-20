#include "WebPage.h"
#include"tinyxml2.h"
#include "mylog.h"
using namespace tinyxml2;
WebPage::WebPage(int id,string& title,string& url,string content)
:_jieba(SplitTool::getPtr())
,_id(id)
,_title(title)
,_url(url)
,_content(content)
{

}

WebPage::~WebPage()
{

}
string WebPage::processDoc(){
    XMLDocument doc;
    XMLElement* root = doc.NewElement("doc");
    doc.InsertEndChild(root);

    XMLElement* docid = doc.NewElement("docid");
    docid->SetText(_id);
    root->InsertEndChild(docid);

    XMLElement* title = doc.NewElement("title");
    title->SetText(_title.c_str());
    root->InsertEndChild(title);

    XMLElement* url = doc.NewElement("url");
    url->SetText(_url.c_str());
    root->InsertEndChild(url);
    LOG_DEBUG("WebPage url :%s",_url.c_str());
    XMLElement* content = doc.NewElement("content");
    content->SetText(_content.c_str());
    root->InsertEndChild(content);
    XMLPrinter printer(0,0);
    doc.Accept(&printer);
    return string(printer.CStr());
}