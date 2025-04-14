#include "MainFrame.h"
#include "Main.h"

#include <wx/wx.h>
#include <curl/curl.h>
#include <json/json.h>


using namespace std;

wxIMPLEMENT_APP(GUI);

//GUI z wxwidgets - rzeczy takie jak wielkość okna
bool GUI::OnInit() {
    MainFrame* mainFrame = new MainFrame("Stacje pogodowe - Projekt JPO");
    mainFrame->SetClientSize(1600, 900);
    mainFrame->Center();
    mainFrame->Show();
    return true;
}

size_t App::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool App::performCurlRequest(const string& url, string& response) {
    //Wyjątek - w momencie braku połączenia wywala błąd
    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            wxLogError("Nie udało się nawiązać połączenia CURL - brak internetu");
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            wxLogError("Problem z połączeniem internetowym (nawiązywaniem połączenia) ", curl_easy_strerror(res));
            return false;
        }

        return true;
    }
    catch (const exception& e) {
        wxLogError("[Błąd CURL] ", e.what());
        return false;
    }
}

//Sczytywanie z JSON'a
bool App::parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot) {
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(jsonResponse, parsedRoot);

    if (!parsingSuccessful) {
        wxLogError("Nie udało się sparsować JSON'a");
        return false;
    }

    return true;
}