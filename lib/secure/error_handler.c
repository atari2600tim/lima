/*
  
(Date unknown) - Jezu@Astaria
Added error_handler() during porting to FluffOS

July 10, 2022 - Jezu@Astaria
- Refactored error_handler().
- Fixed bug in error_handler() that was incorrectly applying verbose_errors.
- Changed debug class for errors generated by wizards to be ':<wiz name>'.  This is so wizzes can selectively enable which wizards they want to receive errors for.

July 03, 2023 - Jezu@SpaceMUD
- Refactored for LIMA

*/
#include <log.h>
#include <daemons.h>

#define DEBUG_BASE_CLASS     "debug"
#define DEBUG_CLASS(x)       DEBUG_BASE_CLASS + "/" + x
#define DEBUG_CLASS_MASTER   DEBUG_CLASS("master")

#define BORDER_CHARACTER     "-"
#define BORDER_WIDTH         60
#define BORDER               repeat_string(BORDER_CHARACTER, BORDER_WIDTH)

#define HIW                  "%^BOLD%^%^WHITE%^"
#define HIR                  "%^BOLD%^%^RED%^"
#define NOR                  "%^RESET%^"

#define CHANNEL_TAG          "%^CHANNEL%^[errors]%^RESET%^"

#define OUTPUT ([ \
  "file" : 0,\
  "line" : 1,\
  "program" : 2,\
  "trace" : 3,\
  "error" : 4,\
  "arguments" : 5,\
  "object" : 6,\
  "interactive" : 7,\
  "context" : 8,\
  "function" : 9,\
  "locals" : 10, \
])

class Error {
  string file;
  int line;
  string program;
  mixed *trace;
  string error;
  mixed *args;
  object ob;
  mixed interactive;
  string uid;
  string func;
  mixed *locals;
  string logfile;
  int caught;
}

class Output {
  string file;
  string line;
  string program;
  string trace;
  string error;
  string arguments;
  string ob;
  string interactive;
  string context;
  string func;
  string locals;
}

private:
  string log_current_error(mapping err_map, int caught);
  string trace_line(object obj, string prog, string file, int line);
  void send_to_channel();

private:
  nosave class Error error_data;
  nosave class Output output_data;
  nosave mapping errors = ([]);
  nosave int has_error = 0;
  nosave string last_error = "";


public string clear_last_error() {
  last_error = "";
}

public string get_last_error() {
  return last_error;
}

private void build_error_data(mapping error_map, int caught)
{
  error_data = new(class Error);;

  error_data.caught      = caught;
  error_data.file        = (string)error_map["file"];
  error_data.line        = (int)error_map["line"];
  error_data.ob          = (object)error_map["object"];
  error_data.program     = (string)error_map["program"];
  error_data.error       = (string)error_map["error"];
  error_data.interactive = this_interactive() || this_user();
  error_data.logfile     = (caught ? LOG_FILE_CATCH : LOG_FILE_RUNTIME);
  error_data.uid         = error_data.interactive ? capitalize((string)error_data.interactive->query_userid() || "(none)") : "(none)";
  error_data.trace       = (mixed*)error_map["trace"];
  if ( arrayp(error_data.trace) && sizeof(error_data.trace) && mapp(error_data.trace[<1]) )
  {
    error_data.func      = error_data.trace[<1]["function"];
    error_data.args      = error_data.trace[<1]["arguments"];
    error_data.locals    = error_data.trace[<1]["locals"];
  }
}

private void build_output_data()
{
  function f;
  output_data = new(class Output);

  f = function(string s, mixed m){
    if ( !stringp(m) )
      m = identify(m);
    return sprintf("\n%s%14s:%s %s", HIW, s, NOR, m);
  };

  output_data.error     = (*f)("Error", error_data.error[0..<2]);
  output_data.ob        = (*f)("Object", error_data.ob);
  output_data.program   = (*f)("Program", error_data.program);
  output_data.func      = (*f)("Function", error_data.func);
  output_data.arguments = (*f)("Arguments", error_data.args);
  output_data.file      = (*f)("File", error_data.file);
  output_data.line      = (*f)("Line", error_data.line);
  
  output_data.trace = sprintf("\n\n\n%sTrace:%s\n\n%s", HIW, NOR,
      implode( 
        map_array( 
          error_data.trace,
          (: sprintf("Line: %O File: %O Function: %O Object: %O Program: %O", 
               (int)$1["line"], 
               (string)$1["file"], 
               (string)$1["function"], 
               (object)$1["object"] || "No object", 
               (string)$1["program"] || "No program") :)
        ), "\n"));

  f = function(int offset, int count) {
    string lines;
    string s = "";

    if ( !error_data.file || !sizeof(stat(error_data.file)) )  
      return s;

    lines = read_file( error_data.file, error_data.line+offset, count);
    if ( !stringp(lines) )
      return s;

    if ( !offset )
      s = sprintf("%s%d:%s  %s", HIR, error_data.line, NOR, lines);
    else
      foreach (string line in explode(lines, "\n"))
        s += sprintf("%d:  %s\n", error_data.line+offset++, line);

    return s; 
  };
  
  output_data.context  = sprintf("\n\n\n%sContext:%s\n\n%s%s%s", 
      HIW, 
      NOR,
      (*f)(-3, 3),
      (*f)(0, 1),
      (*f)(1, 3));
}

protected void error_handler(mapping error_map, int caught) {
  string t;
  mapping enabled_categories;
  string output = "";
  
  last_error = error_map["error"];

  build_error_data(error_map, caught);
  build_output_data();
    
  log_current_error(error_map, caught);
  
  foreach(string cat in (keys(OUTPUT)-({ "context", "trace", "error" })))
  {
    t = fetch_class_member(output_data, OUTPUT[cat]);
    if ( stringp(t) )
      output += t;
  }
  
  output += fetch_class_member(output_data, OUTPUT["trace"]);
  output += fetch_class_member(output_data, OUTPUT["context"]);
  
  if(objectp(error_data.interactive))
  {
    tell(error_data.interactive, 
      sprintf("%s%s\n%s\n", 
        sprintf("\n%s\n%s\n\n%s", 
          BORDER, 
          ctime(), 
          output_data.error), 
        output, 
        BORDER));
  }
  send_to_channel();
}


private void send_to_channel()
{
  CHANNEL_D->deliver_string("errors",
    sprintf("%s (%s) Error logged to %s\n" + "%s %s\n" +
               "%s %s\n",
      CHANNEL_TAG,
      error_data.uid, 
      error_data.logfile, 
      CHANNEL_TAG,
      error_data.error, 
      CHANNEL_TAG,
      master()->trace_line(
        error_data.ob, 
        error_data.program, 
        error_data.file, 
        error_data.line)));
}


protected string log_current_error(mapping err_map, int caught)
{
  object ob;
  string prog, func, output, file;
  int line;
  
  file = file_name(err_map["object"]);
  line = err_map["line"];
  prog = err_map["program"];
  func = err_map["function"];
  if (caught)
  {
    error_data.error = replace_string(error_data.error, "\n", " ");
    return sprintf("[%s] %s line %d: %s\n", ctime(time()), prog, line, error_data.error);
  }

  output  = sprintf("%s\n", ctime(time()));
  output += err_map["error"];
  output += sprintf("program: %s, object: %s line %d\n", prog, file, line);
 
  foreach (mapping new_map in err_map["trace"])
  {
    file = file_name(new_map["object"]);
    line = new_map["line"];
    prog = new_map["program"];
    func = new_map["function"];
    
    output += sprintf("'%15s' in ' %s' ('%s')line %d\n", func, prog, file, line);
  }
  
  write_file(error_data.logfile, output);
}

string trace_line(object obj, string prog, string file, int line)
{
   string ret;
   string objfn = obj ? file_name(obj) : "<none>";

   ret = objfn;
   if (master()->different(objfn, prog))
      ret += sprintf(" (%s)", prog);
   if (file != prog)
      ret += sprintf(" at %s:%d\n", file, line);
   else
      ret += sprintf(" at line %d\n", line);
   return ret;
}

mapping query_error(string name)
{
   /* This MUST be secure */
   if (!check_privilege(1))
      return 0;
   return errors[name];
}
