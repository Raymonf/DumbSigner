//
//  Archiver.cpp
//  AltSign-Windows
//
//  Created by Riley Testut on 8/12/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#include <filesystem>
#include <fstream>

#include "Archiver.hpp"
#include "Error.hpp"

#include <unordered_set>

extern "C" {
#include "zip.h"
#include "unzip.h"
}

#ifdef _WIN32
#include <io.h>
#define access    _access_s
#else
#include <sys/stat.h>
#include <unistd.h>
 #include <dirent.h>
#endif

const int ALTReadBufferSize = 8192;
const int ALTMaxFilenameLength = 512;

#include <sstream>
#include <WinSock2.h>

#define odslog(msg) { std::wstringstream ss; ss << msg << std::endl; OutputDebugStringW(ss.str().c_str()); }

extern std::string StringFromWideString(std::wstring wideString);

#ifdef _WIN32
char ALTDirectoryDeliminator = '\\';
#else
char ALTDirectoryDeliminator = '/';
#endif

#define READ_BUFFER_SIZE 8192
#define MAX_FILENAME 512

namespace fs = std::filesystem;

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

extern std::string replace_all(
    const std::string& str, // where to work
    const std::string& find, // substitute 'find'
    const std::string& replace //      by 'replace'
    );

std::string UnzipAppBundle(std::string filepath, std::string outputDirectory)
{
    if (outputDirectory[outputDirectory.size() - 1] != ALTDirectoryDeliminator) {
        outputDirectory += ALTDirectoryDeliminator;
    }

    unzFile zipFile = unzOpen(filepath.c_str());
    if (zipFile == NULL) {
        throw ArchiveError(ArchiveErrorCode::NoSuchFile);
    }

    FILE* outputFile = nullptr;

    auto finish = [&outputFile, &zipFile](void) {
        if (outputFile != nullptr) {
            fclose(outputFile);
        }

        unzCloseCurrentFile(zipFile);
        unzClose(zipFile);
    };

    unz_global_info zipInfo;
    if (unzGetGlobalInfo(zipFile, &zipInfo) != UNZ_OK) {
        finish();
        throw ArchiveError(ArchiveErrorCode::CorruptFile);
    }

    fs::path payloadDirectoryPath = fs::path(outputDirectory).append("Payload");
    if (!fs::exists(payloadDirectoryPath)) {
        fs::create_directory(payloadDirectoryPath);
    }

    char buffer[ALTReadBufferSize];

    for (int i = 0; i < zipInfo.number_entry; i++) {
        unz_file_info info;
        char cFilename[ALTMaxFilenameLength];

        if (unzGetCurrentFileInfo(zipFile, &info, cFilename, ALTMaxFilenameLength, NULL, 0, NULL, 0) != UNZ_OK) {
            finish();
            throw ArchiveError(ArchiveErrorCode::Unknown);
        }

        std::string filename(cFilename);
        if (startsWith(filename, "__MACOSX")) {
            if (i + 1 < zipInfo.number_entry) {
                if (unzGoToNextFile(zipFile) != UNZ_OK) {
                    finish();
                    throw ArchiveError(ArchiveErrorCode::Unknown);
                }
            }

            continue;
        }

        std::replace(filename.begin(), filename.end(), '/', ALTDirectoryDeliminator);
        filename = replace_all(filename, ":", "__colon__");

        fs::path filepath = fs::path(outputDirectory).append(filename);
        fs::path parentDirectory = filepath.parent_path();

        if (!fs::exists(parentDirectory)) {
            fs::create_directories(parentDirectory);
        }

        if (filename[filename.size() - 1] == ALTDirectoryDeliminator) {
            // Directory
            fs::create_directories(filepath);
        } else {
            // File
            if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
                finish();
                throw ArchiveError(ArchiveErrorCode::Unknown);
            }

            std::string narrowFilepath = StringFromWideString(filepath.c_str());
            odslog("Decompressing to " << narrowFilepath.c_str())

            outputFile = fopen(narrowFilepath.c_str(), "wb");
            if (outputFile == NULL) {
                auto error = GetLastError();
                finish();
                std::cout << "Error: Couldn't open \"" << narrowFilepath << "\". Windows error code was: " << std::to_string(error) << "d" << std::endl;
                fflush(stdout); // immediately show, in case of debugging where the buffer may not be emptied before the throw (which causes the error to never show in the console)
                throw ArchiveError(ArchiveErrorCode::UnknownWrite);
            }

            int result = UNZ_OK;

            do {
                result = unzReadCurrentFile(zipFile, buffer, ALTReadBufferSize);

                if (result < 0) {
                    finish();
                    throw ArchiveError(ArchiveErrorCode::Unknown);
                }

                size_t count = fwrite(buffer, result, 1, outputFile);
                if (result > 0 && count != 1) {
                    finish();
                    throw ArchiveError(ArchiveErrorCode::UnknownWrite);
                }

            } while (result > 0);

            short permissions = (info.external_fa >> 16) & 0x01FF;
            _chmod(narrowFilepath.c_str(), permissions);

            fclose(outputFile);
            outputFile = NULL;
        }

        unzCloseCurrentFile(zipFile);

        if (i + 1 < zipInfo.number_entry) {
            if (unzGoToNextFile(zipFile) != UNZ_OK) {
                finish();
                throw ArchiveError(ArchiveErrorCode::Unknown);
            }
        }
    }

    for (auto& p : fs::directory_iterator(payloadDirectoryPath)) {
        auto filename = p.path().filename().string();

        auto lowercaseFilename = filename;
        std::transform(lowercaseFilename.begin(), lowercaseFilename.end(), lowercaseFilename.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (!endsWith(lowercaseFilename, ".app")) {
            continue;
        }

        auto appBundlePath = payloadDirectoryPath;
        appBundlePath.append(filename);

        auto outputPath = outputDirectory;
        outputPath.append(filename);

        if (fs::exists(outputPath)) {
            fs::remove(outputPath);
        }

        fs::rename(appBundlePath, outputPath);

        finish();

        fs::remove(payloadDirectoryPath);

        return outputPath;
    }

    throw SignError(SignError(SignErrorCode::MissingAppBundle));
}


/*
 * realFilePath: the "real"/original file with proper permissions
 * saveFilePath: the file to put into the ZIP file
 * relativePath: the path for the file in the ZIP file
 */
void WriteFileToZipFile(zipFile* zipFile, fs::path realFilePath, fs::path saveFilePath, fs::path relativePath)
{
    bool isDirectory = fs::is_directory(realFilePath);

    std::string filename = relativePath.string();

    zip_fileinfo fileInfo = {};

    char* bytes = nullptr;
    unsigned int fileSize = 0;

    std::vector<unsigned char> vec;
    if (isDirectory) {
        // Remove leading directory slash.
        if (filename[0] == ALTDirectoryDeliminator) {
            filename = std::string(filename.begin() + 1, filename.end());
        }

        // Add trailing directory slash.
        if (filename[filename.size() - 1] != ALTDirectoryDeliminator) {
            filename = filename + ALTDirectoryDeliminator;
        }
    } else {
        fs::file_status status = fs::status(realFilePath);

        short permissions = (short)status.permissions();
        long shiftedPermissions = 0100000 + permissions;

        uLong permissionsLong = (uLong)shiftedPermissions;

        fileInfo.external_fa = (unsigned int)(permissionsLong << 16L);

        std::ifstream file(saveFilePath.string(), std::ios::binary);
        file.unsetf(std::ios::skipws);
        std::streampos fileSize;
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        vec.reserve(static_cast<size_t>(fileSize));
        vec.insert(vec.begin(),
            std::istream_iterator<unsigned char>(file),
            std::istream_iterator<unsigned char>());
    }

    std::replace(filename.begin(), filename.end(), ALTDirectoryDeliminator, '/');

    if (zipOpenNewFileInZip(*zipFile, (const char*)filename.c_str(), &fileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
        throw ArchiveError(ArchiveErrorCode::UnknownWrite);
    }

    if (zipWriteInFileInZip(*zipFile, vec.data(), vec.size()) != ZIP_OK) {
        zipCloseFileInZip(*zipFile);
        throw ArchiveError(ArchiveErrorCode::UnknownWrite);
    }
}

std::string ZipAppBundle(std::string filepath)
{
    fs::path appBundlePath = filepath;

    auto appBundleFilename = appBundlePath.filename();
    auto appName = appBundlePath.filename().stem().string();

    auto ipaName = appName + ".ipa";
    auto ipaPath = appBundlePath.remove_filename().append(ipaName);

    if (fs::exists(ipaPath)) {
        fs::remove(ipaPath);
    }

    zipFile zipFile = zipOpen((const char*)ipaPath.string().c_str(), APPEND_STATUS_CREATE);
    if (zipFile == nullptr) {
        throw ArchiveError(ArchiveErrorCode::UnknownWrite);
    }

    fs::path payloadDirectory = "Payload";
    fs::path appBundleDirectory = payloadDirectory.append(appBundleFilename.string());
    
    // pass to check for .ldid.* files
    std::unordered_set<std::string> ldidPaths;
    for (auto& entry: fs::recursive_directory_iterator(filepath))
    {
        auto path = entry.path();
        if (startsWith(path.filename().string(), ".ldid."))
        {
	    ldidPaths.insert(path.string());
	}
    }

    for (auto& entry : fs::recursive_directory_iterator(filepath)) {
        auto path = entry.path();
        auto realPath = path;

        // skip if empty or a directory
        if (path.empty() ||
            (ldidPaths.find(path.string()) != ldidPaths.end()) ||
            fs::is_directory(path))
            continue;

        {
            auto ldidPath = path;
            ldidPath.replace_filename(".ldid." + ldidPath.filename().string());
            if (ldidPaths.find(ldidPath.string()) != ldidPaths.end()) {
                std::cout << "[Warning] Using " << ldidPath << " for " << realPath << "." << std::endl;
                path = ldidPath;
            }
        }

        fs::path relativeName = fs::relative(path, filepath);
        auto relativePath = appBundleDirectory / relativeName;
        odslog("Compressing " << path << " to " << relativePath);

        WriteFileToZipFile(&zipFile, realPath, path, relativePath);
    }

    zipClose(zipFile, NULL);

    return ipaPath.string();
}
