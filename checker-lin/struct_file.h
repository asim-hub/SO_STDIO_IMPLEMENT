#define MODE_OPEN 0666
#define SIZE_BUFF 4096
typedef struct _so_file {
	int fd;
	char buff[SIZE_BUFF];
	int dim_buff;
	int back;
	int pos;
	int pos_in_file;
	int end_of_file;
	int fd_error;
} SO_FILE;
