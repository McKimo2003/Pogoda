#pragma once
#include <wx/wx.h>
#include <map>
#include "Graph.h"
#include <vector>

using namespace std;

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString& title);

private:
	//Wybory stacji, wyswietlanie tekstow - wskazniki
	wxChoice* m_stationChoice;
	wxChoice* m_measurementChoice;
	wxChoice* m_startDateTime;
	wxChoice* m_endDateTime;

	wxTextCtrl* m_dataDisplay;

	//Min i max
	wxStaticText* m_statsText;

	//Graf
	Graph* m_graphPanel;

	//Przycisk do filtrowania danych (zakres)
	wxButton* m_filterButton;

	//Czas i dane (do wyboru zakresu)
	vector<wxDateTime> m_dateTimes;
	vector<double> m_dataValues;

	//Wszystkie wartosci (potrzebe do filtrowania - wartosci wszystkie i filtrowane
	vector<double> m_allYData;

	//Id Stacji
	map<wxString, int> m_stationMap;

	//Eventy np: klikanie, zamykanie, wybor
	void onAcceptClick(wxCommandEvent& evt);
	void onClose(wxCloseEvent& evt);
	void onStationSelect(wxCommandEvent& evt);
	void onFilterClick(wxCommandEvent& evt);

	//Obsluga wielu w�tk�w (osobny watek na wyswietlanie danych i osobny na wykres
	void OnDisplayDataThreadEvent(wxThreadEvent& evt);
	void OnUpdateGraphThreadEvent(wxThreadEvent& evt);
};