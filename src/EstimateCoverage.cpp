#include <string>
#include <iostream>
#include <vector>
#include <cmath>
// htslib include
#include "htslib/sam.h"
#include "fastaq/fasta.h"
#include "EstimateCoverage.h"

namespace {
bool LoadKmer(std::string & chr_name_prefix, std::string & kmer_seq, const char * kmer_table, const std::string & chr_name) {
	bool load_chr = false;
	if (chr_name_prefix.empty()) {
		if (Fastaq::FastaLoad(kmer_seq, kmer_table, false, chr_name.c_str())) {
			load_chr = true;
		} else if (Fastaq::FastaLoad(kmer_seq, kmer_table, false, (chr_name + "chr").c_str())) {
			load_chr = true;
			chr_name_prefix = "chr";
		} else {
			std::cerr << "WARNING: Cannot load kmer seqeunces " << chr_name << " from " << kmer_table << std::endl;
			load_chr = false;
		}
	} else {
		if (Fastaq::FastaLoad(kmer_seq, kmer_table, false, (chr_name + chr_name_prefix).c_str())) {
			load_chr = true;
		} else {
			std::cerr << "WARNING: Cannot load kmer seqeunces " << chr_name << " from " << kmer_table << std::endl;
			load_chr = false;
		}
	}

	return load_chr;
}
void CalculateChrCoverage(std::vector<float> & coverages, std::string & chr_name_prefix, 
				samFile * bam_reader, bam_hdr_t *header, hts_idx_t * idx, 
				const char * kmer_table, const char * char_chr_name, const int & min_region_size) {
	const std::string chr_name = char_chr_name;
	std::string kmer_seq;
	const bool load_kmer = LoadKmer(chr_name_prefix, kmer_seq, kmer_table, chr_name);
	size_t pos = 0;

	coverages.clear();

	while (pos != std::string::npos) {
		pos = kmer_seq.find('"', pos);
		if (pos != std::string::npos) {
			int length = 0;
			while(kmer_seq[pos] == '"') {
				++pos;
				++length;
			}
			if (length > min_region_size) {
				const std::string cat_region = chr_name + ":" + std::to_string(pos - length) + '-' +  std::to_string(pos);
				hts_itr_t * ite = sam_itr_querys(idx, header, cat_region.c_str());
				bam1_t * aln = bam_init1();
				unsigned int base_count = 0;
				while (ite && sam_itr_next(bam_reader, ite, aln) >= 0) {
					if (!(aln->core.flag & BAM_FUNMAP)) {
						const uint32_t* pCigar = bam_get_cigar(aln);
						for (uint32_t i = 0; i < aln->core.n_cigar; ++i) {
							const uint32_t op = bam_cigar_op(*(pCigar + i));
							if (op == BAM_CMATCH || op == BAM_CEQUAL || op == BAM_CDIFF)
								base_count += bam_cigar_oplen(*(pCigar + i));
						}
					}
				}
				//std::cerr << cat_region << "\t" << base_count / static_cast<float>(length) << std::endl;
				hts_itr_destroy(ite);
				bam_destroy1(aln);
				coverages.push_back(base_count / static_cast<float>(length));
			}
			
		}
	}
}
}

namespace EstimateCoverage {
int EstimateCoverage(std::vector<float> & coverages, const char * bam_filename, const char * kmer_table) {
	std::string chr_name_prefix;

	samFile * bam_reader = sam_open(bam_filename, "r");
	bam_hdr_t *header;
	header = sam_hdr_read(bam_reader);
	hts_idx_t * idx = sam_index_load(bam_reader,  bam_filename);

	coverages.resize(Human::HumanAutosomeSize + Human::HumanAllosomeSize, 0);

	for (int i  = 0; i < Human::HumanAutosomeSize; ++i) {
		std::vector<float> chr_cov;
		int min_region_size = 20000;
		while (chr_cov.size() < 10 && min_region_size > 2000) {
			min_region_size = min_region_size >> 1;
			chr_cov.clear();
			CalculateChrCoverage(chr_cov, chr_name_prefix, bam_reader, header, idx, kmer_table, Human::HumanAutosome[i], min_region_size);
		}
		float cov_chr_total = 0.0;
		for (std::vector<float>::const_iterator cov_ite = chr_cov.begin(); cov_ite != chr_cov.end(); ++cov_ite) 
			cov_chr_total += *cov_ite;
		coverages[i] = cov_chr_total / static_cast<float>(chr_cov.size());
		std::cerr << Human::HumanAutosome[i] << "\t" << coverages[i] << std::endl;
	}

	for (int i  = 0; i < Human::HumanAllosomeSize; ++i) {
		std::vector<float> chr_cov;
		int min_region_size = 20000;
		while (chr_cov.size() < 10 && min_region_size > 2000) {
			min_region_size = min_region_size >> 1;
			chr_cov.clear();
			CalculateChrCoverage(chr_cov, chr_name_prefix, bam_reader, header, idx, kmer_table, Human::HumanAllosome[i], min_region_size);
		}
		float cov_chr_total = 0.0;
		for (std::vector<float>::const_iterator cov_ite = chr_cov.begin(); cov_ite != chr_cov.end(); ++cov_ite) 
			cov_chr_total += *cov_ite;
		coverages[i + Human::HumanAutosomeSize] = cov_chr_total / static_cast<float>(chr_cov.size());
		std::cerr << Human::HumanAllosome[i] << "\t" << coverages[i + Human::HumanAutosomeSize] << std::endl;
	}

	float all_chr_cov = 0;
	for (int i  = 0; i < Human::HumanAutosomeSize; ++i) {
		all_chr_cov += coverages[i];
	}

	// Clean up
	bam_hdr_destroy(header);
	sam_close(bam_reader);

	return std::round(all_chr_cov / static_cast<float>(Human::HumanAutosomeSize));
}
} //namespace EstimateCoverage