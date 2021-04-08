#include "so_stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "struct_file.h"

SO_FILE *so_fopen(const char *pathname, const char *mode) {
	SO_FILE *file = malloc(sizeof(SO_FILE));

	int i;

	if (file == NULL)
		return NULL;

	/* setez campuri din structura */
	file->fd = -1;
	file->back = -1;
	file->dim_buff = 0;
	file->end_of_file = 0;
	file->pos = 0;
	file->pos_in_file = 0;
	file->fd_error = 0;

	/* tratez cazuri deschidere fisier */

	for (i = 0; i < SIZE_BUFF; i++)
		file->buff[i] = '\0';

	if (strcmp(mode, "r") == 0)
		file->fd = open(pathname, O_RDONLY);

	if (strcmp(mode, "r+") == 0)
		file->fd = open(pathname, O_RDWR);

	if (strcmp(mode, "w") == 0)
		file->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, MODE_OPEN);

	if (strcmp(mode, "w+") == 0)
		file->fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, MODE_OPEN);

	if (strcmp(mode, "a") == 0) {
		file->fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, MODE_OPEN);
		lseek(file->fd, 0, SEEK_END);
	}
	if (strcmp(mode, "a+") == 0) {
		file->fd = open(pathname, O_RDWR | O_APPEND | O_CREAT, MODE_OPEN);
		lseek(file->fd, 0, SEEK_END);
	}
	if (file->fd < 0) {
		free(file);
		return NULL;
	}

	return file;
}

int so_fclose(SO_FILE *stream)
{
	int rc = -1;

	/* scriu date in fisier */
	if (stream == NULL || so_fflush(stream) != 0)
		return SO_EOF;

	/* inchid fisier */
	rc = close(stream->fd);
	/* eliberez memoria */
	free(stream);

	if (rc < 0)
		return SO_EOF;
	return 0;
}

int so_fileno(SO_FILE *stream)
{
	/* returnez file descriptor */
	if (stream == NULL)
		return -1;
	else
		return stream->fd;
}

int so_fflush(SO_FILE *stream)
{
	int recv = 0;
	int total_wr = 0;

	/* scriu datele din buffer in fisier si semnalez
	 * situatiile de eroare/modificari
	 */
	if (stream == NULL)
		return SO_EOF;

	if (stream->back == 1) {
		while (total_wr < stream->dim_buff) {
			recv = write(stream->fd, stream->buff + total_wr,
					stream->dim_buff - total_wr);

			/* cazul in care am ajuns la sfarsitul
			 * fisierului sau a avut loc o erare
			 */
			if (recv < 0) {
				stream->fd_error = 1;
				return SO_EOF;
			}

			total_wr += recv;
		}
		stream->pos = 0;
	}

	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int error = -1;

	/*  scriu buffer in fisier daca ultima operatie a fost write */
	if (stream->back == 1)
		so_fflush(stream);

	/* cazul in care ultima operatie a fost read resetez buffer*/
	if (stream->back == 0) {
		stream->pos = 0;
		stream->dim_buff = 0;
	}

	/* setez cursor in fisier */
	if ((whence == SEEK_CUR) || (whence == SEEK_SET) || (whence == SEEK_END))
		error = lseek(stream->fd, offset, whence);

	if (error != SO_EOF) {
		stream->pos_in_file = error;
		return 0;
	}

	return error;

}

long so_ftell(SO_FILE *stream)
{
	/* returnez pozitie cursor in fisier */
	if (stream == NULL)
		return -1;
	else
		return stream->pos_in_file;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	/* nr total de octeti care trebuie cititi */
	size_t nr_biti = size * nmemb;

	/* octet citit */
	unsigned char char_buff;
	/* offset pentru ptr */
	int nr = 0;

	/* setez ultima operaie facuta */
	stream->back = 0;

	/* citesc bit cu bit din buffer */
	while (nr_biti > 0) {
		char_buff = so_fgetc(stream);

		/* daca am citit un caracter/octet cu succes,
		 * il scriu la adresa data
		 */
		if (SO_EOF) {
			memcpy(ptr + nr, &char_buff, sizeof(char_buff));
			nr++;
			stream->pos_in_file++;
		} else if (nr_biti < size * nmemb && nr_biti != 0) {
			/* cazul in care nu reusesc sa scriu la
			 * adresa data intreg numarul de elemente
			 */
			stream->fd_error = 1;
			return ((size * nmemb) - nr_biti) / size;
		} else {
			return 0;
		}
		/* marchez ca am mai citit un bit */
		nr_biti--;
	}

	return nmemb;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	/* numarul total de biti care trebuie scris */
	int nr_biti = size * nmemb;

	/* offset pentru pointer-ul de unde citesc date */
	int nr = 0;

	/* marchez ultima operatie facuta */
	stream->back = 1;

	/* scriu byte cu byte*/
	while (nr_biti > 0) {
		if (stream->back == 0)
			stream->pos = 0;

		/* setez operatia facuta */
		stream->back = 1;

		/* scriu elemente in buffer, iar cand devine plin il scriu in fisier */
		if (stream->pos == stream->dim_buff && stream->dim_buff == SIZE_BUFF) {
			so_fflush(stream);
			stream->dim_buff = 1;
			memcpy(stream->buff + stream->pos++, (ptr + nr), 1);
		} else {
			stream->dim_buff++;
			memcpy(stream->buff + stream->pos++, (ptr + nr), 1);
		}
		/* marchez bitul citit si cresc pozitia in fisier */
		nr_biti--;
		stream->pos_in_file++;
		nr++;
	}
	/* returnez numarul de elemente scrise */
	if (nr_biti > 0)
		return ((size * nmemb) - nr_biti) / size;
	return nmemb;
}

int so_fgetc(SO_FILE *stream)
{

	if (stream == NULL)
		return SO_EOF;

	/* setez operatia curenta */
	stream->back = 0;

	/* cazul daca buffer-ul e gol si citesc din fisier */
	if (stream->dim_buff == 0 || stream->dim_buff == stream->pos) {
		stream->dim_buff = read(stream->fd, stream->buff, SIZE_BUFF);
		if (stream->dim_buff <= 0) {
			stream->end_of_file = 1;
			stream->fd_error = 1;
		}
		stream->pos = 0;
		stream->back = 0;
	}

	/* returnez valoarea din buffer de pe pozitia cursorului */
	if (stream->dim_buff > 0 && stream->pos < stream->dim_buff) {
		stream->pos++;
		return stream->buff[stream->pos - 1];
	} else {
		return SO_EOF;
	}

	stream->pos_in_file++;
}

int so_fputc(int c, SO_FILE *stream)
{

	if (stream == NULL)
		return -1;

	/* daca ultima operatie facuta a fost read */
	if (stream->back == 0)
		stream->pos = 0;

	/* operatia curenta */
	stream->back = 1;

	/* daca buffer-ul e plin, il resetez */
	if (stream->pos == stream->dim_buff && stream->dim_buff == SIZE_BUFF) {
		so_fflush(stream);
		stream->dim_buff = 1;
		stream->buff[stream->pos++] = c;
		return c;
	}

	/* incrementez cursorul din buffer */
	stream->dim_buff++;

	/* scriu caracterul */
	stream->buff[stream->pos] = c;
	stream->pos++;

	return c;
}

int so_feof(SO_FILE *stream)
{
	/* daca am ajuns la sfarsitul fisierului end_of_file == 1 */
	return stream->end_of_file;
}

int so_ferror(SO_FILE *stream)
{
	/* daca a avut loc o eroare fd_error == 1 */
	return stream->fd_error;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return 0;
}

