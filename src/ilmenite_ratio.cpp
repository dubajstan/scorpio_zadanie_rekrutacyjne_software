#include <iostream>
#include <filesystem>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>


namespace fs = std::filesystem;


double calculateIlmeniteRatio(const fs::path& image_path){
    cv::Mat img = cv::imread(image_path.string(), cv::IMREAD_GRAYSCALE);

    if(img.empty()){
        throw std::runtime_error("Unable to load file " + image_path.string());
    }

    // rozmycie
    cv::Mat blurred;
    cv::GaussianBlur(img, blurred, cv::Size(5,5), 0);

    // progowanie
    int threshold_value = 100;
    cv::Mat mask;
    cv::threshold(blurred, mask, threshold_value, 255, cv::THRESH_BINARY_INV);

    // OBLICZENIA
    int ilmenite_pixels = cv::countNonZero(mask);
    int total_pixels = img.rows * img.cols;
    
    //cv::imshow("Maska - " + image_path.filename().string(), mask);
    //cv::waitKey(0);

    double ratio = (static_cast<double>(ilmenite_pixels) / total_pixels) * 100.0;
    return ratio;
}


int main(){
    fs::path folder_path;
    try{
        folder_path = ament_index_cpp::get_package_share_directory("scorpio_zadanie_rekrutacyjne_software") + "/assets/ilmenite_samples";
    } catch (const std::exception& e){
        std::cerr << "Error: Could not find package"<<std::endl;
        return 1;
    }

    if(!fs::exists(folder_path) || !fs::is_directory(folder_path)){
        std::cerr<<"Error: Folder " << folder_path<<" does not exists"<<std::endl;
        return 1;
    }

    std::vector<fs::path> files;
    for(const auto& entry : fs::directory_iterator(folder_path)){
        files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end());

    for(const auto& file : files){
        double ratio = calculateIlmeniteRatio(file);
        std::cout << file.filename().string() << " - " << std::fixed << std::setprecision(1) << ratio << "%" << std::endl;
    }

    return 0;
}