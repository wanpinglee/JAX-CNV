#ifndef _COUNTKMER_H_
#define _COUNTKMER_H_

#include <getopt.h>
#include <string>


struct CountKmerCml {
	CountKmerCml(const int argc, char** const argv){Parse(argc, argv);}
	CountKmerCml(const char * pInput_jfdb, const char * pInput_fasta, const char * pOutput = NULL, 
			const char * pRegion = NULL, const int input_bin = 1, const bool input_running_length_encoding = false) {
		input_jfdb = pInput_jfdb;
		fasta = pInput_fasta;
		if (pOutput) output = pOutput;
		if (pRegion) region = pRegion;
		bin = input_bin;
		running_length_encoding = input_running_length_encoding;
	}

	bool help = false;

	// i/o
	std::string input_jfdb; // -i --input
	std::string fasta;      // -f --fasta
	std::string output;     // -o --output

	// operation parameters
	std::string region;     // -r --region
	int bin = 1;
	bool running_length_encoding = false; // --rle
	
	// command line
	std::string cmd;

	const char* short_option = "hi:f:o:r:";

	// Help list
	const std::string Help (const char* program) const { return
		std::string("\n") +
		std::string("USAGE: ") + program + std::string(" -i <jellyfish_db> -f <FASTA>\n\n") +
		std::string("	-h --help			Print this help list.\n") +
		std::string("\n") +
		std::string("Input & Output:\n") +
		std::string("	-i --input <jellyfish_db>	Jellyfish created count database.\n") +
		std::string("	-f --fasta <FASTA>		FASTA for kmer lookup.\n") +
		std::string("	-o --output <FILE>		Output file.\n") +
		std::string("\n") +
		std::string("Operations:\n") +
		std::string("	-r --region chr:begin-end	Specify a target region.\n") +
		std::string("	--bin <INT>			Report a result for each # bp. [1]\n") +
		std::string("	--rle				Ouput by running length encoding.\n");
	}

	// Check the required arguments.
	bool CheckArg () const {
		bool ok = true;
		if (bin < 1) {
			std::cerr << "ERROR: --bin <INT> should not smaller than 1." << std::endl;
			ok = false;
		}
		if (bin > 1 && running_length_encoding) {
			std::cerr << "ERROR: --rle only work for --bin 1." << std::endl;
			ok = false;
		}

		return ok && !input_jfdb.empty() && !fasta.empty();
	}

	bool Parse (const int argc, char** const argv) {
		// Record the input command line.
		for (int i = 0; i < argc; i++) cmd += std::string(argv[i]) + " ";
		const struct option long_option[] = {
			{"help", required_argument, NULL, 'h'},
			// i/o
			{"input", required_argument, NULL, 'i'},
			{"fasta", required_argument, NULL, 'f'},
			{"output", required_argument, NULL, 'o'},

			// operation parameters
			{"region", required_argument, NULL, 'r'},
			{"bin", required_argument, NULL, 1},
			{"rle", no_argument, NULL, 2},
			{0,0,0,0}
		};
		int option_index = 0;
		int c = -1;
		while ((c = getopt_long(argc, argv, short_option, long_option, &option_index)) != -1) {
			switch (c) {
				case 'h': help = true; break;
				case 'i': input_jfdb = optarg; break;
				case 'f': fasta = optarg; break;
				case 'o': output = optarg; break;
				case 'r': region = optarg; break;
				case 1: bin = atoi(optarg); break;
				case 2: running_length_encoding = true; break;
				default: std::cerr << "WARNING: Unkonw parameter: " << long_option[option_index].name << std::endl; break;
			}
		}

		if (help) {
			Help(argv[0]);
			return false;
		}

		return CheckArg();
	}
};

class CountKmer {
 public:
	// Constructors
	CountKmer();
	CountKmer(int argc, char** argv);
	CountKmer(const char * pInput_jfdb, const char * pInput_fasta, const char * pOutput = NULL,
			const char * pRegion = NULL, const int input_bin = 1, const bool input_running_length_encoding = false);

	// The function will report kmer count according to the parameter setting.
	// Return: 0 is successful.
	int Run() const;

	// If files are not assinged when declaring the class, you may use the function to assign them.
	void SetParameters(const CountKmerCml & cml);
	void SetParameters(const char * pInput_jfdb, const char * pInput_fasta, const char * pOutput = NULL,
				const char * pRegion = NULL, const int input_bin = 1, const bool input_running_length_encoding = false);
 private:
	CountKmerCml cmdline;
	// Not allow to use copy and assign constructors.
	CountKmer(const CountKmer&);
	CountKmer& operator= (const CountKmer&);
};
#endif