#include "FileCategoryLibrary/FileCategory.h"

#include <filesystem>
#include <iostream>
#include <string>

int main()
{
    const std::filesystem::path testFile{ "example.pdf" };

    const std::string category =
        file_category::getCategory(testFile.extension().string());

    std::cout << testFile.filename().string()
        << " belongs in the "
        << category
        << " category.\n";

    return 0;
}