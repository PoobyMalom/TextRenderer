#include <iostream>
#include <vector>
#include <fstream>
#include "Helpers.h"

int main() {
    // Your code here
    std::vector<char> fontData;

    try {
        std::string filePath = "src/fonts/JetBrainsMono-Regular.ttf";
        fontData = readTTFFile(filePath);
        std::cout << "Successfully read " << fontData.size() << " bytes from " << filePath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "bit one" << fontData[0];
    return 0;
}