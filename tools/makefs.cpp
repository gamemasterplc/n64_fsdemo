#define _CRT_SECURE_NO_WARNINGS //Required to compile on Visual Studio
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
#include <dirent.h>

struct FstEntry {
    uint32_t str_ofs;
    uint32_t data_ofs;
    uint32_t data_len;
};

std::vector<std::string> filenames;

void die(std::string msg)
{
	std::cerr << msg << std::endl;
	exit(1);
}

void PrintUsage(char *name)
{
	std::cerr << "Usage: " << name << " " << "fs_root out_header out_file" << std::endl;
	exit(1);
}

void AddFiles(std::string base_dir, std::string dir_name)
{
    struct dirent *dir_entry;
    DIR *dir_ptr;
    //Try to open the directory
    std::string dir = base_dir + "/" + dir_name + "/";
    dir_ptr = opendir(dir.c_str());
    if (!dir_ptr) {
        die("Failed to open directory " + dir);
    }
    //Loop through directories recursively
    while (dir_entry = readdir(dir_ptr)) {
        std::string name = dir_entry->d_name;
        if (name != "." && name != "..") {
            if (dir_entry->d_type != DT_DIR) {
                std::string file_name;
                if (dir_name != "") {
                    file_name = dir_name + "/" + name;
                } else {
                    file_name = name;
                }
                filenames.push_back(file_name);
            } else {
                AddFiles(base_dir, dir_name + name);
            }
        }
    }
    closedir(dir_ptr);
}

void SetSeek(FILE *file, size_t ofs)
{
    fseek(file, ofs, SEEK_SET);
}

size_t GetFileSize(FILE *file)
{
    size_t old_ofs = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, old_ofs, SEEK_SET);
    return size;
}

void WriteU32(FILE *file, uint32_t value)
{
    uint8_t temp[4];
    temp[0] = value >> 24;
    temp[1] = (value >> 16) & 0xFF;
    temp[2] = (value >> 8) & 0xFF;
    temp[3] = value & 0xFF;
    fwrite(temp, 4, 1, file);
}

void WriteFst(FILE *file, FstEntry *fst, uint32_t count)
{
    for (uint32_t i = 0; i < filenames.size(); i++) {
        WriteU32(file, fst[i].str_ofs);
        WriteU32(file, fst[i].data_ofs);
        WriteU32(file, fst[i].data_len);
    }
}

void CalcStrOffsets(FstEntry *fst, uint32_t base_ofs)
{
    uint32_t str_ofs = base_ofs;
    for (uint32_t i = 0; i < filenames.size(); i++) {
        fst[i].str_ofs = str_ofs;
		//Include null terminator for calculating next string offset
        str_ofs += filenames[i].length()+1;
    }
}

void WriteStrTable(FILE *file)
{
    for (uint32_t i = 0; i < filenames.size(); i++) {
        uint8_t zero = 0;
        //Write string and null terminator
        fwrite(filenames[i].c_str(), 1, filenames[i].length(), file);
        fwrite(&zero, 1, 1, file);
    }
}

void WriteFileData(std::string base_dir, FILE *header_file, FILE *data_file, FstEntry *fst)
{
    uint32_t data_ofs = 0;
    for (uint32_t i = 0; i < filenames.size(); i++) {
		//Calculate path for file to open
        std::string filename = base_dir + "/" + filenames[i];
        FILE *file = fopen(filename.c_str(), "rb");
        if (!file) {
			//Clean up after program if file couldn't open
			fclose(header_file);
            fclose(data_file);
            delete[] fst;
            die("Failed to open " + filename + " " + "for reading.");
        }
		//Read file into temporary buffer exactly as large as file
        uint32_t len = GetFileSize(file);
        uint8_t *temp_buf = new uint8_t[len]();
        fread(temp_buf, 1, len, file);
		//Write buffer to file
        fwrite(temp_buf, 1, len, data_file);
		//Write padding byte to multiple of 2 bytes
        if (ftell(data_file) % 2 != 0) {
            uint8_t zero = 0;
            fwrite(&zero, 1, 1, data_file);
        }
		//Update FST properties
		fst[i].data_ofs = data_ofs;
		fst[i].data_len = len;
		//Calculate next data offset
        data_ofs += (len + 1) & ~1;
		//Close opened file
        fclose(file);
    }
}

void WriteFilesystem(std::string base_dir, std::string out_header, std::string out_datafile)
{
    uint32_t file_count = filenames.size();
	//Open output files
    FILE *out = fopen(out_header.c_str(), "wb");
    if (!out) {
        die("Failed to open " + out_header + " " + "for writing.");
    }
    FILE *data_out = fopen(out_datafile.c_str(), "wb");
    if (!data_out) {
		//Clean up opened file
		fclose(out);
        die("Failed to open " + out_datafile + " " + "for writing.");
    }
    uint32_t str_ofs = 4 + (file_count * sizeof(FstEntry));
    WriteU32(out, file_count); //Write file count
    FstEntry *fst = new FstEntry[file_count]();
	//Write blank FST
    WriteFst(out, fst, file_count);
	//Write file data
    WriteStrTable(out);
    WriteFileData(base_dir, out, data_out, fst);
	//Rewrite FST with new string offsets
	CalcStrOffsets(fst, str_ofs);
	SetSeek(out, 4);
    WriteFst(out, fst, file_count);
	//Clean up after program
    delete[] fst;
    fclose(out);
    fclose(data_out);
}

int main(int argc, char **argv)
{
	if (argc != 4) {
		PrintUsage(argv[0]);
	}
	//Add files to an empty root
    AddFiles(argv[1], "");
    WriteFilesystem(argv[1], argv[2], argv[3]);
	return 0;
}