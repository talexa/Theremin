/*
 * Tim Alexander
 * Testing File Writing System on Mbed Microcontroller
 */
 
#include 'mbed.h'
 
LocalFileSystem local("local"); //Creating local filesystem 
 
int main(){
	/*
	 * FILE * fopen ( const char * filename, const char * mode );
	 * r  -> reading
	 * w  -> writing, warning will overwrite like named files
	 * a  -> append, puts in new file if name does not exist
	 * r+ -> open an existing file for reading and writing
	 * w+ -> create a new file for reading and writing
	 * a+ -> Open for reading or writing at end of file
	 */
	FILE *fp = fopen("/local/output.txt","w");
	fprintf(fp, "Hello World!/n");
	for (int i = 0; i<100; i++){
		fprintf(fp, i);
	}
	fclose(fp);
}
