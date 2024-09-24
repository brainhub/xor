#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>


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
	for( i = 0; i < l; i++ ) {
		a[i] ^= b[i];
	}
}

static int xor_write( FILE *file[3] )  {
	uint64_t abuffer[1024/sizeof(uint64_t)];
	uint64_t bbuffer[1024/sizeof(uint64_t)];

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

			xor_buf( abuffer, bbuffer, m );

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
	FILE *f[3];
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

	return 0;
}

