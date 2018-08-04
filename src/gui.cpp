#include <wx/notebook.h>
#include <wx/gbsizer.h>

#include "gg.hpp" 

// Macros used for binding events to wxWidgets frame functions.
wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, GDB_EVT_STATUS_BAR_UPDATE, GDBFrame::DoStatusBarUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_SOURCE_CODE_UPDATE, GDBFrame::DoSourceCodeUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_LOCALS_UPDATE, GDBFrame::DoLocalsUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_PARAMS_UPDATE, GDBFrame::DoParamsUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_ASSEMBLY_CODE_UPDATE, GDBFrame::DoAssemblyCodeUpdate)
  EVT_COMMAND(wxID_ANY, GDB_EVT_REGISTERS_UPDATE, GDBFrame::DoRegistersUpdate)
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);
bool GDBApp::OnInit() {
  // Determine screen and application dimensions
  long screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
  long screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
  long frame_x = screen_x / 2;
  long frame_y = screen_y / 2;

  // Build command used to initialize application 
  std::string args;
  for (int i = 1; i < wxApp::argc; i++) {
    args.append(wxApp::argv[i]);
    if (i < wxApp::argc - 1) {
      args.append(" ");
    }
  }

  // Create main frame and display 
  GDBFrame * frame = new GDBFrame(GG_FRAME_TITLE, wxApp::argv[0], args, 
      wxPoint((screen_x - frame_x) / 2, (screen_y - frame_y) / 2), 
      wxSize(frame_x, frame_y));
  frame->Show(true);

  // Set top window to be the frame
  SetTopWindow(frame);

  return true;
}

GDBFrame::GDBFrame(const wxString & title, 
    const wxString & clcommand, const wxString & clargs,
    const wxPoint & pos, const wxSize & size) :
  wxFrame(NULL, wxID_ANY, title, pos, size), 
  command(clcommand), args(clargs)
{
  // File section in the menu bar
  wxMenu * menuFile = new wxMenu();
  menuFile->Append(wxID_EXIT);

  // Help section in the menu bar
  wxMenu * menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  // Menu bar on the top 
  wxMenuBar * menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  // Status bar on the bottom
  CreateStatusBar();
  SetStatusText(GDB_STATUS_IDLE);

  // Create notebook (tabbed pane)
  wxNotebook * tabs = new wxNotebook(this, wxID_ANY);

  // Create source code display 
  sourcePanel = new GDBSourcePanel(tabs);
  tabs->AddPage(sourcePanel, "Source");

  // Create assembly code display
  assemblyPanel = new GDBAssemblyPanel(tabs);
  tabs->AddPage(assemblyPanel, "Assembly");
}

void GDBFrame::OnAbout(wxCommandEvent & event) {
  // Display static information
  const char * information = 
    "\nVersion: v"
    GG_VERSION
    "\nAuthors: "
    GG_AUTHORS
    "\nLicense: "
    GG_LICENSE;

  // Display per-instance information 
  std::string text(information);
  text.append("\n\nCommand: ");
  text.append(command.c_str());
  text.append("\nArguments: ");
  text.append(args.c_str());

  // Show message box with the text
  wxMessageBox(text, GG_ABOUT_TITLE, wxOK | wxICON_INFORMATION);
}

GDBSourcePanel::GDBSourcePanel(wxWindow * parent) :
  wxPanel(parent, wxID_ANY) 
{
  // Create main sizer
  wxGridBagSizer * sizer = new wxGridBagSizer();
  SetSizer(sizer);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create source code display and add to sizer
  sourceCodeText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_SOURCE_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(sourceCodeText, 
      wxGBPosition(0, 0), wxGBSpan(2, 1), 
      wxALL | wxEXPAND, 5);

  // Create local variables display and add to sizer
  localsText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_LOCALS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(localsText, 
      wxGBPosition(0, 1), wxGBSpan(1, 1), 
      wxALL | wxEXPAND, 5);

  // Create formal parameters display and add to sizer
  paramsText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_PARAMS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(paramsText, 
      wxGBPosition(1, 1), wxGBSpan(1, 1), 
      wxALL | wxEXPAND, 5);

  // Specify sizer rows and columns that should be growable
  for (int i = 0; i < 2; i++) {
    sizer->AddGrowableRow(i, 1);
    sizer->AddGrowableCol(i, 1);
  }
}

GDBAssemblyPanel::GDBAssemblyPanel(wxWindow * parent) :
  wxPanel(parent, wxID_ANY) 
{
  // Create sizer
  wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(sizer);

  // Style for future text boxes
  long textCtrlStyle = wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL | wxVSCROLL;

  // Create assembly code display and add to sizer
  assemblyCodeText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_ASSEMBLY_CODE),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(assemblyCodeText, 1, wxALL | wxEXPAND, 5);

  // Create registers display and add to sizer
  registersText = new wxTextCtrl(this, wxID_ANY, 
      wxT(GDB_NO_REGISTERS),
      wxDefaultPosition, wxDefaultSize, textCtrlStyle);
  sizer->Add(registersText, 1, wxALL | wxEXPAND, 5);
}


