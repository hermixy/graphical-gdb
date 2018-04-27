#include <wx/wx.h>

#include "pstream.hpp"

// Custom event types sent from the console to the GUI for updates.
const wxEventType gdbEVT_STATUS_BAR_UPDATE = wxNewEventType();
const wxEventType gdbEVT_SOURCE_CODE_UPDATE = wxNewEventType();
const wxEventType gdbEVT_ASSEMBLY_CODE_UPDATE = wxNewEventType();

// Class that represents a GDB process abstraction.
class GDB {
    redi::pstream process; // The bidirectional stream opened to the process
    char buf[BUFSIZ]; // Temporary buffer used to read output and error 
    std::streamsize bufsz; // Number of characters written to temporary buffer at a time
    bool running_program; // Cached value specifying if the user is debugging a program in GDB
    bool running_reset_flag; // Set to true when the value of running_program needs to be updated
    long saved_line_number; // The last known line we executed
  public:
    // Class constructor opens the process.
    GDB(std::vector<std::string> args);

    // Class desctructor closes the process.
    ~GDB(void);

    // Execute the given command by passing it to the process.
    void execute(const char * command);

    // Read whatever output and error is stored in the process.
    // Method will try executing non-blocking reads until ... 
    //  a) the GDB process quits
    //  b) the prompt is detected at the end of the output buffer
    void read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt);

    // Returns true if the GDB process is still alive.
    bool is_alive();

    // Returns true if the GDB process is running/debugging a program.
    bool is_running_program();

    // Gets the source code around where GDB is positioned at.
    std::string get_source_code();
   
    // Gets the local variables in the function GDB is executing.
    std::string get_local_variables();
   
    // Gets the formal parameters (arguments) passed to the function GDB is executing.
    std::string get_formal_parameters();
    
    // Gets the assembly code for the function GDB is in.
    std::string get_assembly_code();

    // Gets GDB's current source code list size.
    long get_source_list_size();
    
    // Gets the current line number GDB is positioned at.
    long get_source_line_number();
  private:
    // Gives option to disable setting internal flags after an execution.
    void execute(const char * command, bool set_flags);

    // Error is merged with output, not recommended for normal use.
    std::string execute_and_read(const char * command);

    // Special case of execute_and_read with an integer argument. 
    std::string execute_and_read(const char * command, long arg);
};

// Class representing the GDB GUI application.
class GDBApp : public wxApp {
  public:
    // Called when our application is initialized via wxEntry().
    virtual bool OnInit();
};

// Class representing the GDB GUI top level display frame.
class GDBFrame : public wxFrame {
  wxStaticText * sourceCodeText; // Displays source code text
  public:
    // Called by GDBApp::OnInit() when it is initializing the top level frame.
    GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size);
  private:
    // Called when the user clicks on the About button in the menu bar.
    void OnAbout(wxCommandEvent & event);

    // Called when the user quits the GUI.
    void OnExit(wxCommandEvent & event);

    // Called when then the console thread posts to the GUI thread that 
    // the status bar should be updated.
    void DoStatusBarUpdate(wxCommandEvent & event);
    
    // Called when the console thread posts to the GUI thread that 
    // the source code display should be updated.
    void DoSourceCodeUpdate(wxCommandEvent & event);

    // Macro to specify that this frame has events that need binding
    wxDECLARE_EVENT_TABLE();
};

// Macros used for binding events to wxWidgets frame functions.
wxBEGIN_EVENT_TABLE(GDBFrame, wxFrame)
  EVT_MENU(wxID_EXIT, GDBFrame::OnExit)
  EVT_MENU(wxID_ABOUT, GDBFrame::OnAbout)
  EVT_COMMAND(wxID_ANY, gdbEVT_STATUS_BAR_UPDATE, GDBFrame::DoStatusBarUpdate)
  EVT_COMMAND(wxID_ANY, gdbEVT_SOURCE_CODE_UPDATE, GDBFrame::DoSourceCodeUpdate)
wxEND_EVENT_TABLE()

// Macro to tell wxWidgets to use our GDB GUI application.
wxIMPLEMENT_APP_NO_MAIN(GDBApp);
