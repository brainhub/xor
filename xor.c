#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <sys/types.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>

static unsigned xor_is_regular( int fd ) {
	struct stat s;
	int r = fstat(fd, &s);
	if( r == -1 )
		return 0;	//false
	return (s.st_mode & S_IFMT) == S_IFREG;
}

static size_t xor_file_size( int fd ) {
	struct stat s;
	int r = fstat(fd, &s);
	if( r == -1 )
		return 0;	//false
	return s.st_size;
}


static int xor_open( const char *a, const char *b, const char *o, FILE *out[3] )  {
    if ((out[0] = fopen(a, "r")) == NULL) {
        printf("ERROR: File (%s) cannot be opened for reading.\n", a);
        return 2;
    }
    if ((out[1] = fopen(b, "r")) == NULL) {
        printf("ERROR: File (%s) cannot be opened for reading.\n", b);
		fclose(out[0]);
        return 2;
    }
    if ((out[2] = fopen(o, "w")) == NULL) {
        printf("ERROR: File (%s) cannot be opened for writng.\n", o);
		fclose(out[0]);
		fclose(out[2]);
        return 2;
    }

	return 0;
}

static void xor_close( FILE *file[3] )  {
	fclose(file[0]);
	fclose(file[1]);
	fclose(file[2]);
}

// a ^= b
static void xor_buf( uint64_t *a, const uint64_t *b, size_t l) {
	size_t i;
	l = (l + sizeof(uint64_t) - 1)/sizeof(uint64_t);	// round up, in 64-bit units
	for( i = 0; i < l; i+=8 ) {
		a[i+0] ^= b[i+0];
		a[i+1] ^= b[i+1];
		a[i+2] ^= b[i+2];
		a[i+3] ^= b[i+3];
		a[i+4] ^= b[i+4];
		a[i+5] ^= b[i+5];
		a[i+6] ^= b[i+6];
		a[i+7] ^= b[i+7];
	}
}

static int xor_mmap_open( const char *sinout, const char *sin, int fd[2] ) {
	int finout;
	int fin;

	if ((finout = open (sinout, O_RDWR)) < 0)  {
		printf("can't open %s for reading and writing", sinout);
		return 1;
	}
	if ((fin = open (sin, O_RDONLY)) < 0)  {
		printf("can't open %s for reading", sin);
		close(finout);
		return 1;
	}
	fd[0] = finout;
	fd[1] = fin;
	return 0;
}

static void xor_mmap_close( int fd[2] ) {
	close(fd[0]);
	close(fd[1]);
	fd[0] = fd[1] = -1;
}

// fd[0] ^= fd[1]
static int xor_mmap_write( int fd[2] ) {
	uint8_t *inout;
	uint8_t *in;
	size_t l = xor_file_size(fd[1]);

	/* mmap the input file */
	if ((in = mmap (0, l, PROT_READ, MAP_SHARED, fd[1], 0)) == (uint8_t *) -1) {
		printf ("mmap error for input");
		return 1;
	}

	/* mmap the output file */
	if ((inout = mmap (0, l, PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0)) == (uint8_t *) -1)  {
		printf ("mmap error for output");
		return 1;
	}

	size_t l64 = l - l%64;

	xor_buf( (uint64_t*)inout, (uint64_t*)in, l64 );

	size_t i;
	for( i=l64; i<l; l++ )  {
		inout[i] ^= in[i];
	}

	munmap( inout, l );
	munmap( in, l );

	return 0;
}


static int xor_write( FILE *file[3] )  {
#define XOR_BUF_SIZE (1024)
	uint64_t abuffer[XOR_BUF_SIZE/sizeof(uint64_t)];
	uint64_t bbuffer[XOR_BUF_SIZE/sizeof(uint64_t)];

	unsigned a_done=0;	// don't ignore EOF, let reads from another file continue
	unsigned b_done=0;

	while(1) {
		size_t a_read = a_done ? 0 : fread(abuffer, 1, sizeof(abuffer), file[0]);
		size_t b_read = b_done ? 0 : fread(bbuffer, 1, sizeof(bbuffer), file[1]);

		// Handle short read
		if( a_read + b_read < sizeof(abuffer) + sizeof(bbuffer) ) {

			size_t m2 = ((a_read > b_read) ? a_read : b_read);	// max
			size_t m = m2;

			memset((uint8_t*)abuffer+a_read, 0, sizeof(abuffer)-a_read);
			memset((uint8_t*)bbuffer+b_read, 0, sizeof(bbuffer)-b_read);

			xor_buf( abuffer, bbuffer, sizeof(abuffer) );

			if( fwrite(abuffer, 1, m, file[2]) != m ) {
				printf("ERROR: Failed to write %jd bytes\n", m );
				return 1;
			}

			if( m < sizeof(abuffer) )
				return 0;	// done writing
		}
		else {
			xor_buf( abuffer, bbuffer, sizeof( abuffer ) );

			if( fwrite(abuffer, 1, sizeof( abuffer ), file[2]) != sizeof( abuffer )) {
				printf("ERROR: Failed to write %jd bytes\n", sizeof( abuffer ));
				return 1;
			}
		}
	}

	return 0;
}

int
main (int argc, char **argv)
{
  char *avalue = NULL;
  char *bvalue = NULL;
  char *ovalue = NULL;
  int c;
	int err;

  opterr = 0;

	if( argc == 1 ) {
		printf("Call as xor -a file1 -b file2 -o out_file1_XOR_file2\n");
		return 1;
	}

  while ((c = getopt (argc, argv, "a:b:o:")) != -1)
    switch (c)
      {
      case 'a':
        avalue = optarg;
        break;
      case 'b':
        bvalue = optarg;
        break;
      case 'o':
        ovalue = optarg;
        break;
      case '?':
		if (optopt == 'a' || optopt == 'b' || optopt == 'o')  {
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			return 1;
		}
		fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        return 1;
      default:
		fprintf (stderr, "invalid options\n");
        return 1;
      }

	//printf ("input1 = %s, input2 = %s, output = %s\n", avalue, bvalue, ovalue);

//	for (index = optind; index < argc; index++)
//		printf ("Non-option argument %s\n", argv[index]);

	int fd[2] = {-1,-1};

	/* Special mode: one of input files is the same as output */
	if( strcmp( avalue, ovalue )==0 ) {
		err = xor_mmap_open( ovalue, bvalue, fd );
		if( err )
			return err;
	}
	if( strcmp( bvalue, ovalue )==0 ) {
		err = xor_mmap_open( ovalue, avalue, fd );
		if( err )
			return err;
	}

	/* Other conditions for mmmap: must be regular files */
	if( !xor_is_regular(fd[0]) || !xor_is_regular(fd[1]) )
		xor_mmap_close(fd);

	/* Other conditions for mmmap: first file must be larger */
	if( xor_file_size(fd[0]) <  xor_file_size(fd[1]) )
		xor_mmap_close(fd);

	if( fd[0] != -1 )  {
		// mmap case
		err = xor_mmap_write(fd);
		if( err )
			return err;
		xor_mmap_close(fd);
	}
	else {
		// general case
		FILE *f[3];
		err = xor_open( avalue, bvalue, ovalue, f );
		if( err ) {
			return err;
		}
		err = xor_write( f );
		if( err ) {
			xor_close(f);
			return err;
		}
		xor_close(f);
	}

	return 0;
}

