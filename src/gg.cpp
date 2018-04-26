#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

#include "gg.hpp"

#define GDB_PROMPT "(gdb) "

#define GDB_QUIT "quit"
#define GDB_PROGRAM_STATUS "info program"
#define GDB_SOURCE_LIST "list"
#define GDB_ASSEMBLY_LIST "disassemble"

#define GDB_STATUS_IDLE "GDB is idle."
#define GDB_STATUS_RUNNING "GDB is currently running a program."

// GDB process mode should be completely self-contained. 
const redi::pstreams::pmode GDB_PMODE = 
  redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;

// Helper function for determining if a string ends with a certain value.
bool string_ends_with(std::string const & str, std::string const & ending) {
  if (ending.size() > str.size()) 
    return false;
  return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

// Helper function for determining if a string contains a value.
bool string_contains(std::string const & str, std::string const & value) {
  return str.find(value) != std::string::npos;
}

// Class constructor opens the process.
GDB::GDB(std::vector<std::string> args) : 
  process("gdb", args, GDB_PMODE), 
  running_program_flag(false), running_program(false) {}

// Class desctructor closes the process.
GDB::~GDB() {
  process.close();
}

// Execute the given command by passing it to the process.
void GDB::execute(const char * line) {
  if (is_alive() && line) {
    // Pass line directly to process
    process << line << std::endl;
    running_program_flag = true;
  }
}

// Read whatever output and error is stored in the process.
// Method will try executing non-blocking reads until ... 
//  a) the program quits
//  b) it detects the prompt at the end of the stdout buffer
void GDB::read_until_prompt(std::ostream & output_buffer, std::ostream & error_buffer, bool trim_prompt) {
  // Do non-blocking reads
  bool hit_prompt = false;
  while (is_alive() && !hit_prompt) {
    // Read process's error stream and append to error string
    while (bufsz = process.err().readsome(buf, sizeof(buf))) {
      std::string error(buf, bufsz);
      error_buffer << error << std::flush;
    }

    // Read process's output stream and append to output string 
    while (bufsz = process.out().readsome(buf, sizeof(buf))) {
      std::string output(buf, bufsz);

      // Signal a break if output ends with the prompt
      if (string_ends_with(output, GDB_PROMPT)) {
        hit_prompt = true;
        // Trim the prompt from the output if specified
        if (trim_prompt) {
          output.erase(output.size() - strlen(GDB_PROMPT), output.size());
        }
      }

      output_buffer << output << std::flush;
    }
  }
}

// Returns true if the GDB process is still alive.
bool GDB::is_alive() {
  bool exited = 
    process.out().rdbuf()->exited() || // Check process output buffer
    process.err().rdbuf()->exited();   // Check process error buffer
  return !exited;
}

// PRIVATE (error is discarded, not recommended for normal use)
std::string GDB::get_execution_output(const char * line) {
  // Create stream buffers
  std::ostringstream output_buffer;
  std::ostringstream error_buffer; // Will be discarded

  // Call line in GDB 
  process << line << std::endl;

  // Get result of command
  read_until_prompt(output_buffer, error_buffer, true);

  return output_buffer.str();
}

// Returns true if the GDB process is running/debugging a program.
bool GDB::is_running_program() {
  if (running_program_flag) {
    // Collect program status output
    std::string program_status = get_execution_output(GDB_PROGRAM_STATUS);

    // Output with "not being run" only appears when GDB is not running anything
    running_program = !string_contains(program_status, "not being run");

    // Set flag to false, execute will reset it
    running_program_flag = false;
  }

  return running_program; 
}

std::string GDB::get_source_code() {
  // A running program has source code that can be printed
  if (is_running_program()) {
    return get_execution_output(GDB_SOURCE_LIST);
  }

  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
}

std::string GDB::get_assembly_code() {
  // A running program has assembly that can be disassembled
  if (is_running_program()) {
    return get_execution_output(GDB_ASSEMBLY_LIST);
  }
  
  // Not relevant for programs that aren't running
  std::string empty;
  return empty;
}

// Called when our application is initialized via wxEntry().
bool GDBApp::OnInit() {
  // Determine screen and application dimensions
  long screen_x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
  long screen_y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
  long frame_x = screen_x / 2;
  long frame_y = screen_y / 2;

  // Create main frame and display 
  GDBFrame * frame = new GDBFrame("GDB Display", 
      wxPoint((screen_x - frame_x) / 2, (screen_y - frame_y) / 2), 
      wxSize(frame_x, frame_y));
  frame->Show(true);

  // Set top window to be the frame
  SetTopWindow(frame);

  return true;
}

// Called by GDBApp::OnInit() when it is initializing the top level frame.
GDBFrame::GDBFrame(const wxString & title, const wxPoint & pos, const wxSize & size) :
  wxFrame(NULL, wxID_ANY, title, pos, size) 
{
  // File section in the menu bar
  wxMenu * menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT);

  // Help section in the menu bar
  wxMenu * menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  // Menu bar on the top 
  wxMenuBar * menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  // Status bar on the bottom
  CreateStatusBar();
  SetStatusText(GDB_STATUS_IDLE);
}

// Called when the user clicks on the About button in the menu bar.
void GDBFrame::OnAbout(wxCommandEvent & event) {
  wxMessageBox("This is a wxWidget's Hello world sample",
               "About Hello World", wxOK | wxICON_INFORMATION);
}

// Called when the user quits the GUI.
void GDBFrame::OnExit(wxCommandEvent & event) {
  Close(true);
}

// Called when then the console thread posts to the GUI thread
// that a status bar update should be made.
void GDBFrame::DoStatusBarUpdate(wxCommandEvent & event) {
  SetStatusText(event.GetString());
}

void update_console_and_gui(GDB & gdb) {
  // Read from GDB to populate buffer
  gdb.read_until_prompt(std::cout, std::cerr, true);

  // Create event to update GUI status bar
  wxCommandEvent * sbu_event = 
    new wxCommandEvent(gdbEVT_STATUS_BAR_UPDATE);
  sbu_event->SetString(
      gdb.is_running_program() ? GDB_STATUS_RUNNING : GDB_STATUS_IDLE);

  // Queue events if application has been initialized on separate thread
  GDBApp * app = (GDBApp *) wxTheApp;
  wxWindow * window = app->GetTopWindow();
  if (window) { // Window will be null if GDBApp::OnInit() hasn't been called
    wxEvtHandler * window_handler = window->GetEventHandler();
    window_handler->QueueEvent(sbu_event);
  }
}

void open_console(int argc, char ** argv) {
  // Convert raw C string to STL string 
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) {
    char * arg = argv[i];
    std::string argstr(arg);
    args.push_back(argstr);
  }

  // Create instance of GDB
  GDB gdb(args);

  // Display gdb introduction to user 
  update_console_and_gui(gdb);

  while (gdb.is_alive()) {
    // Read one line from stdin to process (blocking)
    char * buf_input = readline(GDB_PROMPT);

    // A null pointer signals input EOF 
    if (!buf_input) {
      // Print quit prompt since it isn't printed from input 
      std::cout << GDB_QUIT << std::endl;

      // Execute the quit command
      gdb.execute(GDB_QUIT);

      // Display the result of the quit command
      update_console_and_gui(gdb);
      break;
    }

    // Handle empty input separately from regular command
    if (strlen(buf_input)) {
      // Add input to our CLI history
      add_history(buf_input);

      // Execute the command that we read in 
      gdb.execute(buf_input);

      // Display the result of the command and the next prompt
      update_console_and_gui(gdb);
    }

    // Delete the input buffer, free up memory
    delete buf_input;
  }
}

void open_gui(int argc, char ** argv) {
  wxEntry(argc, argv);
}

int main(int argc, char ** argv) {
  // Run GUI on detached thread; main thread will post events to it
  std::thread gui(open_gui, 1, argv);
  gui.detach();

  // Main thread opens console to accept user input 
  open_console(argc, argv);

  return 0;
}

