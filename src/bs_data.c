#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "bsheader.h"
#include "adler32.h"
#include "block.h"

RETURN_CODE parse_args(int argc, char** argv,
					   char** ppLeftChecksumFilename,
					   char** ppRightChecksumFilename,
					   char** ppTargetFilename,
					   char** ppOutputFilename,
					   char** ppUserdata) {

	// Default values
	*ppLeftChecksumFilename = NULL;
	*ppRightChecksumFilename = NULL;
	*ppTargetFilename = NULL;
	*ppOutputFilename = NULL;
	*ppUserdata = NULL;

	printf("Parse arguments");

	opterr = 0;
	int c;
	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{ "left", 		required_argument, 0, 'l' },
			{ "right", 		required_argument, 0, 'r' },
			{ "target", 	required_argument, 0, 't' },
			{ "output",		required_argument, 0, 'o' },
			{ "user-data", 	required_argument, 0, 'u' },
			{ 0, 0, 0, 0 } };
		c = getopt_long(argc, argv, "l:r:t:o:u:", long_options, &option_index);
		if (c == -1) { break; }
		switch (c) {
		case 'l':
			*ppLeftChecksumFilename = optarg;
			printf("-- Left checksum file: %s\n", *ppLeftChecksumFilename);
			break;
		case 'r':
			*ppRightChecksumFilename = optarg;
			printf("-- Right checksum file: %s\n", *ppRightChecksumFilename);
			break;
		case 't':
			*ppTargetFilename = optarg;
			printf("-- Target: %s\n", *ppTargetFilename);
			break;
		case 'o':
			*ppOutputFilename = optarg;
			printf("-- Output data file: %s\n", *ppOutputFilename);
			break;
		case 'u':
			*ppUserdata = optarg;
			printf("-- User data: %s\n", *ppUserdata);
			break;
		default:
			printf("Invalid option\n");
		}
	}

	CHECK_PTR_RETURN(ppLeftChecksumFilename, ILLEGAL_ARG);
	CHECK_PTR_RETURN(ppRightChecksumFilename, ILLEGAL_ARG);
	CHECK_PTR_RETURN(ppTargetFilename, ILLEGAL_ARG);
	CHECK_PTR_RETURN(ppOutputFilename, ILLEGAL_ARG);

	return EXIT_SUCCESS;
}

int checkHeaders(BSHeader* pHeaderLeft, BSHeader* pHeaderRight) {
	CHECK_PTR_RETURN(pHeaderLeft, ILLEGAL_ARG);
	CHECK_PTR_RETURN(pHeaderRight, ILLEGAL_ARG);
	if (pHeaderLeft->version != pHeaderRight->version && pHeaderRight->version != 1) {
		return -1;
	}
	if (pHeaderLeft->blockSize != pHeaderRight->blockSize) {
		return -2;
	}
	if (pHeaderLeft->totalSize != pHeaderRight->totalSize) {
		return -3;
	}
	if (pHeaderLeft->type != pHeaderRight->type && pHeaderRight->type != CHECKSUM) {
		return -4;
	}
	return 0;
}

uint64_t getBufferSize (BSHeader* pHeader, uint64_t blockId) {
	return (blockId == (getBlockCount(pHeader)-1)) ? getLastBlockSize(pHeader) : pHeader->blockSize;
}

void* readBlock(BSHeader* pHeader, uint64_t blockId, uint64_t size, FILE* pSource) {
	void* pOut = malloc(size);
	if (pOut != NULL) {
		if (fseek(pSource, blockId * pHeader->blockSize, SEEK_SET) == 0) {
			if (fread(pOut, size, 1, pSource) == 1) {
				return pOut;
			}
		}
	}
	AUTOFREE(pOut);
	return pOut;
}

RETURN_CODE bs_data(int argc, char** argv) {

	int rc = 0;
	char* pLeftChecksumFilename = NULL;
	char* pRightChecksumFilename = NULL;
	char* pTargetFilename = NULL;
	char* pOutputFilename = NULL;
	char* pUserdata = NULL;

	FILE* pLeftChecksumFile = NULL;
	FILE* pRightChecksumFile = NULL;
	FILE* pTargetFile = NULL;
	FILE* pOutputFile = NULL;

	BSHeader* pLeftHeader = NULL;
	BSHeader* pRightHeader = NULL;
	BSHeader* pOutputHeader = NULL;

	void* pBuffer = NULL;

	TRY
	if ((rc = parse_args(argc, argv,
						 &pLeftChecksumFilename,
						 &pRightChecksumFile,
						 &pTargetFilename,
						 &pOutputFilename,
						 &pUserdata)) != 0) {
		THROW("Invalid arguments", 1);
	}

	// Open files
	if ((pLeftChecksumFile = fopen(pLeftChecksumFilename, "r")) == NULL) {
		THROW("Error opening left checksum file", OPEN_ERROR);
	}
	if ((pRightChecksumFile = fopen(pRightChecksumFilename, "r")) == NULL) {
		THROW("Error opening right checksum file", OPEN_ERROR);
	}

	pLeftHeader = readHeader(pLeftChecksumFile);
	CHECK_PTR_THROW(pLeftHeader, "Error reading header for left checksum");

	pRightHeader = readHeader(pRightChecksumFile);
	CHECK_PTR_THROW(pRightHeader, "Error reading header for right checksum");

	// Check
	if ((rc = checkHeaders(pLeftHeader, pRightHeader)) != 0) {
		THROW("Checksum files are incompatible", rc);
	}

	// Create header
	if (pUserdata == NULL) {
		pUserdata = pRightHeader->pUserData;
	}
	pOutputHeader = newHeader(DATA, pRightHeader->totalSize, pRightHeader->blockSize, pUserdata);
	printHeaderInformation(pOutputHeader, TRUE);

	// Open output file
	if ((pOutputFile = fopen(pOutputFilename, "w")) == NULL) {
		THROW("Error opening output data file", OPEN_ERROR);
	}

	// Write header
	if ((rc = writeHeader(pOutputFile, pOutputFile)) != 0) {
		THROW("Error writing checksum", rc);
	}

	// Open target file
	if ((pTargetFile = fopen(pTargetFilename, "r")) == NULL) {
		THROW("Error opening target", OPEN_ERROR);
	}

	uint64_t currentBlock;
	uint32_t fromChecksum, toChecksum;
	uint64_t blockCount = getBlockCount(pRightHeader);
	printf("Block count: %"PRIu64"\n", blockCount);
	for (currentBlock = 0; currentBlock < blockCount; currentBlock++) {
		if (fread(&fromChecksum, sizeof(uint32_t), 1, pLeftChecksumFile) != 1 ||
			fread(&toChecksum,   sizeof(uint32_t), 1, pRightChecksumFile) != 1) {
			THROW("Cannot read from checksum file", 100);
		}
		if (fromChecksum != toChecksum) {
			printf("Checksum are different for block %"PRIu64", %"PRIu32" != %"PRIu32"\n",
					currentBlock, fromChecksum, toChecksum);
			uint64_t size = getBufferSize(pOutputHeader, currentBlock);
			pBuffer = readBlock(pOutputHeader, currentBlock, size, pTargetFile);
			CHECK_PTR_THROW(pBuffer, "Error reading buffer");
			if (pBuffer != NULL) {
				printf("  Writing block %"PRIu64" -> size: %"PRIu64"\n", currentBlock, size);
				// Check if block hav valid checksum
				uint32_t currentChecksum = getChecksum(pBuffer, size);
				if (currentChecksum != toChecksum) {
					printf("  Target file does not march left checksum: %"PRIu32" != %"PRIu32"\n", currentChecksum, toChecksum);
					THROW("Invalid checksum for block", 9);
				}
				if (fwrite(&currentBlock, sizeof(uint32_t), 1, pOutputFile) != 1 ||
					fwrite(pBuffer, size, 1, pOutputFile) != 1) {
					THROW("Error writing in data file", WRITE_ERROR);
				}
				fflush(pOutputFile);
			}
		}
	}

	CATCH

	FINALLY

	AUTOFREE(pBuffer);
	AUTOFREE(pLeftHeader);
	AUTOFREE(pRightHeader);
	AUTOFREE(pOutputHeader);
	AUTOCLOSE(pLeftChecksumFile);
	AUTOCLOSE(pRightChecksumFile);
	AUTOCLOSE(pOutputFile);
	AUTOCLOSE(pTargetFile);
	return exceptionId;
}

int main(int argc, char** argv) {
	printf("binary-sync: data\n");
	return bs_data(argc, argv);
}