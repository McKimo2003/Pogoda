#pragma once
#include <wx/wx.h>
#include <vector>

using namespace std;

class Graph : public wxPanel
{
private: //Deklaracja zmiennych potrzebnych do tworzenia wykresu
    bool m_hasData = false;
    vector<double> m_xData;
    vector<double> m_yData;

public: //redklaracja funkcji, przydatnych przy tworzeniu wykresów
    Graph(wxWindow* parent);

    void SetData(const vector<double>& x, const vector<double>& y);
    void OnPaint(wxPaintEvent& event);

    const vector<double>& getY()const {
        return m_yData;
    }
    const vector<double>& getX()const {
        return m_xData;
    }
};