#pragma once
#include <wx/wx.h>
#include <curl/curl.h>
#include <json/json.h>

using namespace std;

//Klasa odpowiedzialna za GUI - deklaracja, wykonanie
class GUI : public wxApp
{
public:
    virtual bool OnInit();
};

//Klasa odpowiedzialna za ³¹czenie siê z baz¹ danych - deklaracja funkcji
class App : public wxApp {
public:
    static bool parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot);
    static bool performCurlRequest(const string& url, string& response);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};