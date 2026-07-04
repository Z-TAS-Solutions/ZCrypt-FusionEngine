#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

int main() {
  std::cout << "Choose an installation option:\n";
  std::cout << "  [1] Install\n";
  std::cout << "  [2] Clean Install\n";
  std::cout << "  [3] Cancel\n\n";
  std::cout << "Enter selection (1-3): ";

  char choice;
  std::cin >> choice;

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  if (choice == '2') {
    std::cout << "\nPurging project package cache allocations...\n";
    std::string rm_cmd = "conan remove \"*\" -c";
    std::system(rm_cmd.c_str());
  } else if (choice == '3' || (choice != '1' && choice != '2')) {
    std::cout << "bleh...\n";

    return 0;
  }

  std::cout << "\nExecuting Conan configuration pipeline...\n";
  int result = std::system("conan install . --build=missing");

  if (result != 0) {
    std::cerr << "Pipeline installation failed execution.\n";

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return result;
  }

  std::cout << "Build space environments generated successfully!\n";

  std::cout << "\nPress Enter to exit...";
  std::cin.get();
  return 0;
}
