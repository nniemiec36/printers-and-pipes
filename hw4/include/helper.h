#define PROMPT "imp> "
#define HELP "\nUSAGE: Imprimer\n\n %s\n", \
		"Misc. commands: [help] [quit]\n" \
		"Config. commands: [type | file_type] [printer | printer_name file_type]\n" \
		"\t[conversion | file_type1 file_type2 conversion_program | arg1 arg2 ...]\n" \
		"Info. commands: [printers] [jobs]\n" \
		"Spooling commands: [print | file_name | printer1 printer2 ...] [cancel | job_number]\n" \
		"\t[pause | job_number] [resume | job_number] [disable | printer_name] [enable | printer_name]\n" \
		"   help       Displays this help menu.\n" \
		"   quit       Terminates the program.\n\n" \
		"   Configuration commands:\n" \
		"   type       Declares file_type to be a file type supported by the program.\n" \
		"   printer    Declares the existence of a printer named printer_name, capable of printing file_type.\n" \
		"   conversion Declares a program to convert file_type1 to file_type2 with provided args.\n\n" \
		"   Informational commands:\n" \
		"   printers   Prints a report of the current state of printers.\n" \
		"   jobs       Prints a report of the current state of jobs.\n\n" \
		"   Spooling commands:\n" \
		"   print      Sets up a job for printing file_name.\n" \
		"   cancel     Sets up a job for printing file_name\n" \
		"   pause 	   Pauses a command that is currently being processed.\n" \
		"   resume     Resumes a job that was previously paused.\n" \
		"   disable    Sets the state of a specified printer to \"disabled\".\n" \
		"   enable  Sets the state of a specified printer to \"enabled\".\n\n"

		/*
 * Structure to represent information about a particular file type.
 */
typedef struct printer {
  char * name;       /* Filename extension for this type. */
  char * type; /* Pointer to file type associated with printer. */
  PRINTER_STATUS status;
} PRINTER;

typedef struct job {
  int index;
  char * type;
  char * file_name;
  JOB_STATUS status;
  int eligible;
  struct tm * creation;
  PRINTER * printer;
  CONVERSION ** conversions;
  int pgid;
} JOB;

PRINTER *printers[MAX_PRINTERS];
JOB *jobs[MAX_JOBS];

int parse_arguments(FILE * in, char * line, FILE * out);

int create_printer(char * name, char * file_name);
void free_printers();
int find_printer_type(char * name, char * file_name);
int get_next_index_printers();
void print_printers(FILE * out);
PRINTER *enable_printer(char * name);
PRINTER *disable_printer(char * name);

int create_job(char * type_name, char ** eligible_printers);
void free_jobs();
int get_next_index_jobs();
int find_job(int job_number);
int update_job_status(int job_number, char * status);
void print_jobs(FILE * out);

int create_conversion(char * file1, char * file2, char ** cmd_and_args);
int available_jobs();
int start_job(JOB * job);
PRINTER * get_eligible_printer(int eligible, char * file_name);

void handler();

