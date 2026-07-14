#include "FileCategoryLibrary/FileCategory.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace
{
    // Class that finds and moves movie files
    class MovieOrganizer
    {
    public:
        // Find movie files inside the given directory
        [[nodiscard]]
        std::vector<std::filesystem::path> findMovies(
            const std::filesystem::path& sourceDirectory) const
        {
            // Stores all movie paths that are found
            std::vector<std::filesystem::path> movies;

            // Stores filesystem errors without crashing program
            std::error_code error;

            // Check if source directory actually exist
            if (!std::filesystem::exists(sourceDirectory, error))
            {
                std::cerr << "The source path does not exist:\n"
                          << sourceDirectory << '\n';

                return movies;
            }

            // Check if source path is actually a directory
            if (!std::filesystem::is_directory(sourceDirectory, error))
            {
                std::cerr << "The source path is not a directory:\n"
                          << sourceDirectory << '\n';

                return movies;
            }

            // Skip folders program does not have permission for
            const auto options =
                std::filesystem::directory_options::skip_permission_denied;

            // Start searching source and all folders inside it
            std::filesystem::recursive_directory_iterator iterator(
                sourceDirectory,
                options,
                error);

            // Represents the end of directory search
            const std::filesystem::recursive_directory_iterator endIterator;

            // Check if directory could not be opened
            if (error)
            {
                std::cerr << "Unable to open the source directory: "
                          << error.message() << '\n';

                return movies;
            }

            // Keep checking files until search reaches end
            while (iterator != endIterator)
            {
                // Get current file or folder being checked
                const std::filesystem::directory_entry& entry = *iterator;

                // Stores error for current directory entry
                std::error_code entryError;

                // Add file if it is normal file and movie type
                if (entry.is_regular_file(entryError) &&
                    !entryError &&
                    isMovieFile(entry.path()))
                {
                    movies.push_back(entry.path());
                }

                // Move iterator to next file or folder
                iterator.increment(error);

                // Skip item if error happened while searching
                if (error)
                {
                    std::cerr << "Skipped an inaccessible item: "
                              << error.message() << '\n';

                    error.clear();
                }
            }

            // Sort movie paths in alphabet order
            std::sort(movies.begin(), movies.end());

            // Return all found movies
            return movies;
        }

        // Move one movie into destination directory
        bool moveMovie(
            const std::filesystem::path& moviePath,
            const std::filesystem::path& destinationDirectory) const
        {
            // Stores filesystem errors
            std::error_code error;

            // Check if selected movie exists and is normal file
            if (!std::filesystem::exists(moviePath, error) ||
                !std::filesystem::is_regular_file(moviePath, error))
            {
                std::cerr << "The selected movie does not exist:\n"
                          << moviePath << '\n';

                return false;
            }

            // Create destination folders if they do not exist
            std::filesystem::create_directories(
                destinationDirectory,
                error);

            // Stop if destination folder could not be made
            if (error)
            {
                std::cerr << "Could not create the destination directory: "
                          << error.message() << '\n';

                return false;
            }

            // Combine destination folder with movie file name
            const std::filesystem::path destinationPath =
                destinationDirectory / moviePath.filename();

            // Stop if same file name already exist there
            if (std::filesystem::exists(destinationPath, error))
            {
                std::cerr << "A file with the same name already exists:\n"
                          << destinationPath << '\n';

                return false;
            }

            // Try normal move first
            std::filesystem::rename(
                moviePath,
                destinationPath,
                error);

            // Move worked without error
            if (!error)
            {
                std::cout << "Movie moved successfully to:\n"
                          << destinationPath << '\n';

                return true;
            }

            // Clear move error before trying copy method
            error.clear();

            // Copy movie to destination if normal move failed
            const bool copied = std::filesystem::copy_file(
                moviePath,
                destinationPath,
                std::filesystem::copy_options::none,
                error);

            // Stop if copying movie failed
            if (!copied || error)
            {
                std::cerr << "Could not copy the movie: "
                          << error.message() << '\n';

                return false;
            }

            // Clear error before removing original
            error.clear();

            // Remove original after copy was successful
            const bool originalRemoved =
                std::filesystem::remove(moviePath, error);

            // Warn if copied movie but original did not remove
            if (!originalRemoved || error)
            {
                std::cerr
                    << "The movie was copied, but the original could not "
                       "be removed.\n"
                    << "Copied file:\n"
                    << destinationPath << '\n'
                    << "Original file:\n"
                    << moviePath << '\n';

                return false;
            }

            // Tell user movie move was successful
            std::cout << "Movie moved successfully to:\n"
                      << destinationPath << '\n';

            return true;
        }

    private:
        // Check if a file is one of the movie file types
        [[nodiscard]]
        bool isMovieFile(
            const std::filesystem::path& filePath) const
        {
            // List of accepted movie extensions
            static const std::unordered_set<std::string> movieExtensions{
                ".mp4",
                ".mkv",
                ".avi",
                ".mov",
                ".m4v",
                ".wmv",
                ".webm",
                ".mpeg",
                ".mpg"
            };

            // Get extension from the file path
            std::string extension =
                filePath.extension().string();

            // Make extension letters all lowercase
            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char character)
                {
                    // Change one character to lowercase
                    return static_cast<char>(
                        std::tolower(character));
                });

            // Get category from FileCategoryLibrary
            const std::string category =
                file_category::getCategory(extension);

            // Return true if category and extension are video
            return category == "Videos" &&
                   movieExtensions.find(extension) !=
                       movieExtensions.end();
        }
    };
}

int main()
{
    // Smart pointer owns MovieOrganizer object
    std::unique_ptr<MovieOrganizer> movieOrganizer =
        std::make_unique<MovieOrganizer>();

    // Stores source path typed by user
    std::string sourceInput;

    // Ask user where movie files are located
    std::cout << "Enter the path containing the movies:\n> ";
    std::getline(std::cin, sourceInput);

    // Turn string input into filesystem path
    const std::filesystem::path sourceDirectory{
        sourceInput
    };

    // Search source directory for movie files
    const std::vector<std::filesystem::path> movies =
        movieOrganizer->findMovies(sourceDirectory);

    // Stop program if no movies was found
    if (movies.empty())
    {
        std::cout << "No movie files were found.\n";
        return 0;
    }

    // Display heading before movie list
    std::cout << "\nMovies found:\n";

    // Print each movie with a number
    for (std::size_t index = 0; index < movies.size(); ++index)
    {
        std::cout << index + 1
                  << ". "
                  << movies[index].filename().string()
                  << '\n';
    }

    // Ask user which movie they want moved
    std::cout << "\nEnter the number of the movie to move: ";

    // Stores selected movie number
    std::size_t selection{};

    // Check if user typed a valid number
    if (!(std::cin >> selection))
    {
        std::cerr << "The selection must be a number.\n";
        return 1;
    }

    // Check if number is inside the movie list
    if (selection == 0 || selection > movies.size())
    {
        std::cerr << "The selected movie number is invalid.\n";
        return 1;
    }

    // Remove leftover newline from number input
    std::cin.ignore(
        std::numeric_limits<std::streamsize>::max(),
        '\n');

    // Stores destination path typed by user
    std::string destinationInput;

    // Ask where selected movie should be moved
    std::cout << "Enter the destination path:\n> ";
    std::getline(std::cin, destinationInput);

    // Turn destination string into filesystem path
    const std::filesystem::path destinationDirectory{
        destinationInput
    };

    // Get selected movie path from vector
    const std::filesystem::path& selectedMovie =
        movies[selection - 1];

    // Display which movie was selected
    std::cout << "\nSelected movie:\n"
              << selectedMovie << "\n\n";

    // Try moving selected movie to destination
    if (!movieOrganizer->moveMovie(
            selectedMovie,
            destinationDirectory))
    {
        std::cerr << "The movie could not be moved.\n";
        return 1;
    }

    // Program completed successful
    return 0;
}
