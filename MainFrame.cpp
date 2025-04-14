#include "MainFrame.h"
#include "Main.h"
#include <curl/curl.h>
#include <json/json.h>
#include <wx/wx.h>

#include <thread>

using namespace std;

//Zmienne do obsługi wielu wątków  - ID eventów
enum {
	ID_EVENT_DISPLAY_DATA = wxID_HIGHEST + 1,
	ID_EVENT_UPDATE_GRAPH
};

MainFrame::MainFrame(const wxString& title) :wxFrame(nullptr, wxID_ANY, title) { //GUI
	wxPanel* panel = new wxPanel(this); //panel główny

	//Wątki
	Bind(wxEVT_THREAD, &MainFrame::OnDisplayDataThreadEvent, this, ID_EVENT_DISPLAY_DATA);
	Bind(wxEVT_THREAD, &MainFrame::OnUpdateGraphThreadEvent, this, ID_EVENT_UPDATE_GRAPH);

	//BoxSizer(obszar)
	wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags flags = wxSizerFlags().Left().Border(wxALL, 10); //wyrównanie oraz jaka odleglosc od kazdej krawędzi

	//Text statyczny nr:1
	wxStaticText* statictext1 = new wxStaticText(panel, wxID_ANY, "Wybierz stację pogodową");
	statictext1->SetBackgroundColour(*wxLIGHT_GREY); //zmiana teskstu (tło)
	boxSizer->Add(statictext1, flags); //dodawanie tekstu do BoxSizera

	//nawiazywanie polaczenia z baza danych json + dodawanie wyboru stacji
	setlocale(LC_CTYPE, "Polish");

	string api_url = "https://api.gios.gov.pl/pjp-api/rest/station/findAll"; //adres do serwera (JSONA)
	string response;
	wxArrayString choices1;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	if (App::performCurlRequest(api_url, response)) {
		Json::Value root;
		if (App::parseJsonResponse(response, root)) {
			//dodawanie stacji z bazy danych do listy
			for (Json::Value::const_iterator outer = root.begin(); outer != root.end(); outer++) {
				wxString station = wxString::FromUTF8(root[outer.index()]["stationName"].asCString());
				int stationId = root[outer.index()]["id"].asInt();
				choices1.Add(station);
				m_stationMap[station] = stationId;
			}
		}
		else {
			wxLogMessage("Błąd pobierania danych");
		}
	}
	curl_global_cleanup();

	//dodawanie stacji do pola wyboru
	m_stationChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices1, wxCB_SORT);
	m_stationChoice->Bind(wxEVT_CHOICE, &MainFrame::onStationSelect, this);
	boxSizer->Add(m_stationChoice, flags);

	//Tekst statyczny nr: 2
	wxStaticText* statictext2 = new wxStaticText(panel, wxID_ANY, "Wybierz stanowiska pomiarowe");
	statictext2->SetBackgroundColour(*wxLIGHT_GREY);
	boxSizer->Add(statictext2, flags);

	//Wybor tego co moze wyswietlac stacja (sensory) - początkowo puste
	wxArrayString choices2;
	m_measurementChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices2, wxCB_SORT);
	boxSizer->Add(m_measurementChoice, flags);

	//Przycisk do zatwierdzania wyboru stacji oraz sensorów do pokazania danych
	wxButton* accept = new wxButton(panel, wxID_ANY, "Zatwierdź", wxDefaultPosition, wxSize(150, 30), wxBU_EXACTFIT);
	accept->Bind(wxEVT_BUTTON, &MainFrame::onAcceptClick, this);
	boxSizer->Add(accept, flags);

	//Text statyczny nr:3
	wxStaticText* statictext3 = new wxStaticText(panel, wxID_ANY, "Dane wyświetlane tekstowo (od najnowszych wpisów):");
	statictext3->SetBackgroundColour(*wxCYAN); //zmiana teskstu (tło)
	boxSizer->Add(statictext3, flags); //dodawanie tekstu do BoxSizera

	m_dataDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(1400, 400), wxTE_MULTILINE | wxTE_READONLY);
	boxSizer->Add(m_dataDisplay, flags);

	//Text statyczny nr:4
	wxStaticText* statictext4 = new wxStaticText(panel, wxID_ANY, "Dane wyświetlane na wykresie, w celu zaktualizowania filtrów oraz danych na wykresie naciśnij przycisk 'Filtruj zakres danych'");
	statictext4->SetBackgroundColour(*wxGREEN); //zmiana teskstu (tło)
	boxSizer->Add(statictext4, flags); //dodawanie tekstu do BoxSizera

	//Graf - nowy BoxSizer zeby wszystko się zmieściło pod poprzednim boxSizerze
	wxBoxSizer* timeSizer = new wxBoxSizer(wxHORIZONTAL);

	//Dodawanie filtrów daty do wykresu - graf
	m_startDateTime = new wxChoice(panel, wxID_ANY); //początkowa data filtru
	m_endDateTime = new wxChoice(panel, wxID_ANY); //końcowa data filtru
	m_filterButton = new wxButton(panel, wxID_ANY, "Filtruj zakres danych"); //przycisk filtrujucy

	timeSizer->Add(new wxStaticText(panel, wxID_ANY, "Od:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	timeSizer->Add(m_startDateTime, 0, wxALL, 5);
	timeSizer->Add(new wxStaticText(panel, wxID_ANY, "Do:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	timeSizer->Add(m_endDateTime, 0, wxALL, 5);
	timeSizer->Add(m_filterButton, 0, wxALL, 5);

	boxSizer->Add(timeSizer, flags);

	m_filterButton->Bind(wxEVT_BUTTON, &MainFrame::onFilterClick, this);

	//Max oraz Min dla wykresu - po filtracji
	m_statsText = new wxStaticText(panel, wxID_ANY, "Min: - | Max: -");
	timeSizer->Add(m_statsText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	//wyswietlanie grafu
	m_graphPanel = new Graph(panel);
	boxSizer->Add(m_graphPanel, flags);

	//wylaczanie programu
	this->Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);

	//Przypisywanie panelu zeby dzialal z boxSizerem (wartosci typu odleglosci od krawedzi dla wszystkich obiektow)
	panel->SetSizer(boxSizer);
	boxSizer->SetSizeHints(this);
}



//Naciśnięcie przycisku Akceptuj
void MainFrame::onAcceptClick(wxCommandEvent& evt) {
	wxString selectedStation = m_stationChoice->GetStringSelection();
	wxString selectedParam = m_measurementChoice->GetStringSelection();

	//Jeżeli nie ma wybranych stacji lub sensorów
	if (selectedStation.IsEmpty() || selectedParam.IsEmpty()) {
		wxLogMessage("Wybierz stację i parametr.");
		return;
	}

	//Nadawanie ID stacji
	int stationId = m_stationMap[selectedStation];

	//Pobieranie danych ze stacji
	string sensorUrl = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + to_string(stationId);
	string sensorResponse;

	//Błąd pobierania listy sensorów
	if (!App::performCurlRequest(sensorUrl, sensorResponse)) {
		wxLogMessage("Nie udało się pobrać listy sensorów.");
		return;
	}

	//Błąd parsowania JSON'a - JSON nie odpowiada
	Json::Value sensorsRoot;
	if (!App::parseJsonResponse(sensorResponse, sensorsRoot)) {
		wxLogMessage("Błąd parsowania JSON (sensory).");
		return;
	}

	//jeżeli nie ma pasującego sensora - ID zostaje -1 i nie ma sensorów dla tej stacji
	int sensorId = -1;

	//Dodawanie sensorów do listy
	for (const auto& sensor : sensorsRoot) {
		string paramName = sensor["param"]["paramName"].asString();
		if (paramName == selectedParam.ToStdString()) {
			sensorId = sensor["id"].asInt();
			break;
		}
	}

	//Nie znaleziono sensora - błąd
	if (sensorId == -1) {
		wxLogMessage("Nie znaleziono sensora dla wybranego parametru.");
		return;
	}

	//Pobieranie danych sensora
	string dataUrl = "https://api.gios.gov.pl/pjp-api/rest/data/getData/" + to_string(sensorId);
	string dataResponse;

	//Nie ma listy pomiarów
	if (!App::performCurlRequest(dataUrl, dataResponse)) {
		wxLogMessage("Nie udało się pobrać danych pomiarowych.");
		return;
	}

	//Błąd parsowania JSON'a
	Json::Value dataRoot;
	if (!App::parseJsonResponse(dataResponse, dataRoot)) {
		wxLogMessage("Błąd parsowania JSON (dane).");
		return;
	}

	//Zapisywanie wartości z JSON'a do wartości w tablicy
	const Json::Value& values = dataRoot["values"];
	wxString display;

	//Tworzenie pola vectora(tablicy) z danymi osie X i Y
	vector<double> xData, yData;

	m_dateTimes.clear();
	m_startDateTime->Clear();
	m_endDateTime->Clear();
	m_allYData.clear();

	//Przypisywanie danych do parametrow i daty + tworzenie wykresów
	for (int index = 0; index < values.size(); ++index) {
		const auto& v = values[index];
		string date = v["date"].asString();
		string value = v["value"].isNull() ? "0" : v["value"].asString();
		display += wxString::Format("Data: %s  |  Wartość%s\n", date, value);

		//Data (do zakresów)
		wxDateTime dt;
		bool parsed = false;

		//Sprawdzanie prawidłowości parsowania danych na różne sposoby (w zależności od formatu JSON'a)
		if (!parsed && dt.ParseISOCombined(date)) {
			parsed = true;
		}

		if (!parsed && dt.ParseDateTime(date)) {
			parsed = true;
		}

		if (!parsed) {
			wxString wxDateStr = wxString::FromUTF8(date);
			parsed = dt.ParseFormat(wxDateStr, "%Y-%m-%d %H:%M:%S");
		}

		//Sprawdza czy JSON ma zawartość i jeśli tak, dodaje date do listy  wstawia jako możliwy wybór w filtrze
		if (!v["value"].isNull()) {
			wxDateTime dt;
			bool parsed = false;

			if (dt.ParseISOCombined(date)) parsed = true;
			else if (dt.ParseDateTime(date)) parsed = true;
			else if (dt.ParseFormat(wxString::FromUTF8(date), "%Y-%m-%d %H:%M:%S")) parsed = true;

			if (parsed) {
				m_dateTimes.push_back(dt);
				wxString formatted = dt.FormatISOCombined(' ');
				m_startDateTime->Append(formatted);
				m_endDateTime->Append(formatted);

				double val = v["value"].asDouble();
				xData.push_back(index);
				yData.push_back(val);
				m_allYData.push_back(val);
				index++;
			}
			else {
				wxLogMessage("Błąd z parsowaniem daty: %s", date);
			}
		}
	}

	//Dodanie obsługi wielu wątków (jeden wątek odpowiedzialny za wykres, drugi wątek odpowiedzialny za tekst z danymi
	if (m_dateTimes.size() == m_allYData.size() && !m_allYData.empty()) {
		thread textThread([=]() { //Wyświetlanie w formie tekstu - tworzy nowy wątek który genereuje tekst
			wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, ID_EVENT_DISPLAY_DATA); 
			evt->SetString(display);
			wxQueueEvent(this, evt);
		});

		textThread.detach(); //Odłączenie od głównego wątku (działa w tle)
		
	}
	else {
		wxLogMessage("Dane nie są kompletne. - podaj zakres dat"); //W razie problemu z danymi
	}
}

//Po naciśnięciu filtrowania (wybor przedzialu czasowego)
void MainFrame::onFilterClick(wxCommandEvent& evt) {
	int startIdx = m_startDateTime->GetSelection();
	int endIdx = m_endDateTime->GetSelection();

	//W razie nie wybrania wartości
	if (startIdx == wxNOT_FOUND || endIdx == wxNOT_FOUND) {
		wxLogMessage("Proszę wybrać daty początku i końca");
		return;
	}

	//Sprawdzanie indeksów i dat
	if (startIdx >= m_dateTimes.size() || endIdx >= m_dateTimes.size()) {
		wxLogMessage("Błąd: Indeks daty poza zakresem");
		return;
	}

	wxDateTime start = m_dateTimes[startIdx];
	wxDateTime end = m_dateTimes[endIdx];

	//Zabezpieczenie przed ustawieniem źle daty (na odwrót)
	if (start > end) {
		wxLogMessage("Data początkowa musi być wcześniejsza niż końcowa");
		return;
	}

	//Tworzenie wektora z "przefiltrowanymi" danymi
	vector<double> filteredX, filteredY;
	wxString filteredDisplay;
	int index = 0;

	//Sprawdzenie czy vektory maja te same wielkosci (czy sa zgodne)
	if (m_dateTimes.size() != m_allYData.size()) {
		wxLogMessage("Błąd: Niezgodność rozmiarów tablic dat i wartości");
		return;
	}

	//Sprawdzenie czy i jest zgodne z zakresem vektora od daty
	for (size_t i = 0; i < m_dateTimes.size(); ++i) {
		if (i >= m_allYData.size()) {
			wxLogMessage("Błąd: Indeks poza zakresem tablicy wartości");
			break;
		}

		//Jeżli jest ok to dodajemy do przefiltrowanych wartości
		if (m_dateTimes[i].IsBetween(start, end) || m_dateTimes[i].IsEqualTo(start) || m_dateTimes[i].IsEqualTo(end)) {
			filteredX.push_back(index++);
			filteredY.push_back(m_allYData[i]);

			wxString formatted = m_dateTimes[i].Format("%Y-%m-%d %H:%M:%S");
			filteredDisplay += wxString::Format("Data: %s  |  Wartość: %.2f\n",
				formatted, m_allYData[i]);
		}
	}

	//Jeżeli przefiltrowane X będzie puste - błąd
	if (filteredX.empty()) {
		wxLogMessage("Brak danych w podanym zakresie.");
		return;
	}



	m_dataDisplay->SetValue(filteredDisplay);

	thread graphThread([=]() { //Wyświetlanie w formie grafu - tworzy nowy wątek który genereuje graf (przekazuje dane xData i yData i wysyła do GUI)
		wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, ID_EVENT_UPDATE_GRAPH);
		evt->SetPayload(make_pair(filteredX, filteredY)); //Budowanie wykresu
		wxQueueEvent(this, evt);
	});

	graphThread.detach(); //Odłączenie od głównego wątku (działa w tle)

	//Min i max wykresu - po dodaniu filtracji odpowiedniej
	if (!filteredY.empty()) {
		auto minmax = minmax_element(filteredY.begin(), filteredY.end());
		wxString stats = wxString::Format("Min: %.2f | Max: %.2f", *minmax.first, *minmax.second);
		m_statsText->SetLabel(stats);
	}
}

//Zamykanie programu (zabawa)
void MainFrame::onClose(wxCloseEvent& evt) {
	wxLogMessage("Zamykanie programu...");
	evt.Skip();
}

//Po wyborze stacji - pobieranie sensorów
void MainFrame::onStationSelect(wxCommandEvent& evt) {
	wxString selectedStation = m_stationChoice->GetStringSelection();
	int stationId = m_stationMap[selectedStation];
	string url = "https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + to_string(stationId);
	string response;

	//Pobieranie danych z JSON'a (jakie pomiary może mieć dana stacja)
	if (App::performCurlRequest(url, response)) {
		Json::Value root;
		if (App::parseJsonResponse(response, root)) {
			wxArrayString measurements;
			for (Json::Value::const_iterator it = root.begin(); it != root.end(); ++it) {
				string paramName = (*it)["param"]["paramName"].asString();
				measurements.Add(wxString::FromUTF8(paramName));
			}
			m_measurementChoice->Clear();
			m_measurementChoice->Append(measurements);
		}
		else {
			wxLogMessage("Błąd parsowania JSON (sensory)");
		}
	}
	else {
		wxLogMessage("Błąd pobierania danych sensorycznych");
	}
}

//Wątek - aktualizacja tekstu przy wątku
void MainFrame::OnDisplayDataThreadEvent(wxThreadEvent& evt) {
	wxString text = evt.GetString();
	m_dataDisplay->SetValue(text);
}

//Wątek - aktualizacja grafu przy wątku
void MainFrame::OnUpdateGraphThreadEvent(wxThreadEvent& evt) {
	auto dataPair = evt.GetPayload<pair<vector<double>, vector<double>>>();
	m_graphPanel->SetData(dataPair.first, dataPair.second);
}