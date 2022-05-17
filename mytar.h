int list_cmd(char* fileName, char *directories[], int numDirectories,
     int verboseBool, int strictBool);

int extract_cmd(char* fileName, char *directories[], int numDirectories,
     int verboseBool, int strictBool);

int create_cmd(int verboseBool, int strictBool, int num_paths,
    char *outfile_name, char **paths);
