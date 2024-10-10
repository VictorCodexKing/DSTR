#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <regex>
#include <windows.h>  // For clearing screen on Windows

using namespace std;
using namespace chrono;

using Ms = chrono::milliseconds;
using Mu = chrono::microseconds;
using Timer = chrono::high_resolution_clock;

// Templated resizable array class (similar to std::vector functionality)
template <typename T>
class ResizableArray {
private:
    T* data;
    int capacity;
    int size;

    // Resize the array when it's full
    void resize() {
        int newCapacity = capacity * 2;
        T* newData = new T[newCapacity];
        for (int i = 0; i < size; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    // Constructor
    explicit ResizableArray(int initialCapacity = 10)
        : size(0), capacity(initialCapacity) {
        if (capacity <= 0) {
            throw invalid_argument("Capacity must be greater than 0");
        }
        data = new T[capacity];
    }

    // Destructor
    ~ResizableArray() {
        delete[] data;
    }

    // Add an element
    void add(const T& element) {
        if (size == capacity) {
            resize();
        }
        data[size++] = element;
    }

    // Get an element by index
    T get(int index) const {
        if (index < 0 || index >= size) {
            throw out_of_range("Index out of bounds");
        }
        return data[index];
    }

    // Get the current size
    int getSize() const {
        return size;
    }

    // Binary Search
    int binarySearch(const T& element) const {
        int low = 0;
        int high = size - 1;
        while (low <= high) {
            int mid = (low + high) / 2;
            if (data[mid] == element) {
                return mid; // Found the element
            }
            else if (data[mid] < element) {
                low = mid + 1;
            }
            else {
                high = mid - 1;
            }
        }
        return -1; // Element not found
    }

    // Insertion Sort
    void insertionSort() {
        for (int i = 1; i < size; i++) {
            T key = data[i];
            int j = i - 1;
            while (j >= 0 && data[j] > key) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }
};

void loadWordsFromFile(const string& filename, ResizableArray<string>& words);
void loadReviewsAndRatingsFromCSV(const string& filename, ResizableArray<string>& reviews, ResizableArray<int>& userRatings, int& totalWords);
void clearScreen();
string cleanWord(const string& word);
int roundSentimentScore(double score);
double calculateSentimentScore(int positiveCount, int negativeCount);
void countWordsInReview(const string& review, const ResizableArray<string>& positiveWords, const ResizableArray<string>& negativeWords, ResizableArray<string>& positiveWordsFound, ResizableArray<string>& negativeWordsFound, int& positiveCount, int& negativeCount);
void generateSentimentAnalysis(const ResizableArray<string>& reviews, const ResizableArray<int>& userRatings, const ResizableArray<string>& positiveWords, const ResizableArray<string>& negativeWords, int reviewNumber);
int countOccurrences(const ResizableArray<int>& indices, int index);
void displayWordsWithOccurrences(ResizableArray<string>& words, ResizableArray<int>& counts);
void countWordsInReviewBinary(const string& review, const ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, const ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts);
void performSemanticAnalysis(const ResizableArray<string>& reviews, const ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, const ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts, int totalWords, Timer::time_point start);
void displaySummary(int totalReviews, int totalWords, int positiveWordCount, int negativeWordCount, Ms timeElapsed);
void displayMenu(ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts, ResizableArray<string>& reviews, ResizableArray<int>& userRatings, int totalWords, Ms timeElapsed);


// Function to load words from a file into a dynamic array
void loadWordsFromFile(const string& filename, ResizableArray<string>& words) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Failed to open file: " + filename);
    }
    string word;
    while (file >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);  // Convert words to lowercase
        words.add(word);
    }
    file.close();
}

// Function to handle proper CSV parsing (quotes, commas, etc.)
void loadReviewsAndRatingsFromCSV(const string& filename, ResizableArray<string>& reviews, ResizableArray<int>& userRatings, int& totalWords) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Failed to open file: " + filename);
    }

    string line;
    bool isFirstLine = true;  // Skip the first line if it's a header row

    // Regex for matching CSV fields with quotes or commas
    regex csvPattern("\"([^\"]*)\",(\\d+)|([^,]+),(\\d+)");

    while (getline(file, line)) {
        // Skip the header line
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        smatch match;
        if (regex_search(line, match, csvPattern)) {
            string review = match[1].str().empty() ? match[3].str() : match[1].str();
            int rating = stoi(match[2].str().empty() ? match[4].str() : match[2].str());

            reviews.add(review);   // Add review to the array
            userRatings.add(rating);  // Add user rating to the array

            // Count words in the review
            stringstream ss(review);
            string word;
            while (ss >> word) {
                totalWords++;
            }
        }
    }
    file.close();
}

// Function to clear the console (works for both Windows and Unix-like systems)
void clearScreen() {
    system("cls");  // Windows command to clear the console
}



// Function to clean and normalize a word (convert to lowercase and remove punctuation)
string cleanWord(const string& word) {
    string cleanWord;
    for (char c : word) {
        if (isalnum(c)) {
            cleanWord += tolower(c);
        }
    }
    return cleanWord;
}

// Function to round sentiment score to the nearest whole number
int roundSentimentScore(double score) {
    return static_cast<int>(round(score));
}

// Function to calculate sentiment analysis score
double calculateSentimentScore(int positiveCount, int negativeCount) {
    int N = positiveCount + negativeCount;
    if (N == 0) return 3.0;  // Neutral score when no words found

    int rawScore = positiveCount - negativeCount;
    int minRawScore = -N;
    int maxRawScore = N;
    double normalizedScore = double(rawScore - minRawScore) / (maxRawScore - minRawScore);
    return 1 + 4 * normalizedScore;  // Map score to a 1-5 range
}

// Function to count positive and negative words in a review
void countWordsInReview(const string& review, const ResizableArray<string>& positiveWords, const ResizableArray<string>& negativeWords, ResizableArray<string>& positiveWordsFound, ResizableArray<string>& negativeWordsFound, int& positiveCount, int& negativeCount) {
    stringstream ss(review);
    string word;
    positiveCount = 0;
    negativeCount = 0;

    while (ss >> word) {
        word = cleanWord(word);  // Clean and normalize the word

        // Check if it's a positive or negative word
        if (positiveWords.binarySearch(word) != -1) {
            positiveWordsFound.add(word);
            positiveCount++;
        }
        else if (negativeWords.binarySearch(word) != -1) {
            negativeWordsFound.add(word);
            negativeCount++;
        }
    }
}


// Function to perform sentiment analysis on a specific review by review number
void generateSentimentAnalysis(const ResizableArray<string>& reviews, const ResizableArray<int>& userRatings, const ResizableArray<string>& positiveWords, const ResizableArray<string>& negativeWords, int reviewNumber) {
    // Start timer
    auto start = Timer::now();

    if (reviewNumber <= 0 || reviewNumber > reviews.getSize()) {
        cout << "Invalid review number!\n";
        return;
    }

    int positiveCount = 0;
    int negativeCount = 0;
    ResizableArray<string> positiveWordsFound;
    ResizableArray<string> negativeWordsFound;

    // Get the review text and perform analysis
    const string& review = reviews.get(reviewNumber - 1);
    countWordsInReview(review, positiveWords, negativeWords, positiveWordsFound, negativeWordsFound, positiveCount, negativeCount);

    // Calculate sentiment score and round it
    double sentimentScore = calculateSentimentScore(positiveCount, negativeCount);
    int roundedSentimentScore = roundSentimentScore(sentimentScore);

    // Determine sentiment rating
    std::string sentimentRating;
    if (roundedSentimentScore >= 4) {
        sentimentRating = "Positive";
    }
    else if (roundedSentimentScore <= 2) {
        sentimentRating = "Negative";
    }
    else {
        sentimentRating = "Neutral";
    }

    // End timer
    auto end = Timer::now();
    auto duration = duration_cast<microseconds>(end - start);

    // Clear the screen for smoother output
    clearScreen();

    // Display results in the requested format
    cout << "Review #" << reviewNumber << "\n";
    cout << "Comment: " << review << "\n";
    cout << "\nPositive Words = " << positiveCount << "\n";
    for (int i = 0; i < positiveWordsFound.getSize(); i++) {
        cout << "~ " << positiveWordsFound.get(i) << "\n";
    }

    cout << "\nNegative Words = " << negativeCount << "\n";
    for (int i = 0; i < negativeWordsFound.getSize(); i++) {
        cout << "~ " << negativeWordsFound.get(i) << "\n";
    }

    cout << "\nSentiment Score Rating: " << roundedSentimentScore << " (" << sentimentRating << ")\n";
    cout << "Rating given by user: " << userRatings.get(reviewNumber - 1) << "\n";
    cout << "Time Taken to Calculate: " << duration.count() << "us\n";

}



// Count occurrences in ResizableArray
int countOccurrences(const ResizableArray<int>& indices, int index) {
    int count = 0;
    for (int i = 0; i < indices.getSize(); i++) {
        if (indices.get(i) == index) {
            count++;
        }
    }
    return count;
}

// Display words with occurrences
void displayWordsWithOccurrences(ResizableArray<string>& words, ResizableArray<int>& counts) {
    clearScreen();
    for (int i = 0; i < words.getSize(); i++) {
        int count = countOccurrences(counts, i);
        if (count > 0) {
            cout << words.get(i) << "(" << count << ") | ";
        }
    }
    cout << "NULL\n\n";  // Print NULL at the end of the list

    // Wait for user to press Enter to continue
    cout << "Press Enter to Continue...";
    cin.ignore();  // Ignore the newline left in the buffer from the previous input
    cin.get();     // Wait for user to press Enter
    clearScreen(); // Clear the screen before returning to the main menu
}


// Count positive/negative words in review using binary search
void countWordsInReviewBinary(const string& review, const ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, const ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts) {
    stringstream ss(review);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);  // Convert to lowercase
        int posIndex = positiveWords.binarySearch(word);
        int negIndex = negativeWords.binarySearch(word);
        if (posIndex != -1) {
            posCounts.add(posIndex);
        }
        if (negIndex != -1) {
            negCounts.add(negIndex);
        }
    }
}

// Perform Semantic Analysis (binary search-based)
void performSemanticAnalysis(const ResizableArray<string>& reviews, const ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, const ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts, int totalWords, Timer::time_point start) {
    cout << "Performing Binary Search.....\nReview #";
    for (int i = 0; i < reviews.getSize(); i++) {
        // Overwrite the same line to show updated review number
        cout << "\rReview #" << i + 1 << flush;
        countWordsInReviewBinary(reviews.get(i), positiveWords, posCounts, negativeWords, negCounts);
    }
}

// Display the summary of the analysis
void displaySummary(int totalReviews, int totalWords, int positiveWordCount, int negativeWordCount, Ms timeElapsed) {
    clearScreen();
    cout << "==== Summary ====\n";
    cout << "Number of Reviews: " << totalReviews << endl;
    cout << "Total Words: " << totalWords << endl;
    cout << "Positive Words: " << positiveWordCount << endl;
    cout << "Negative Words: " << negativeWordCount << endl;
    cout << "Time Elapsed: " << timeElapsed.count() << " ms\n";
}

// Main menu with input validation
void displayMenu(ResizableArray<string>& positiveWords, ResizableArray<int>& posCounts, ResizableArray<string>& negativeWords, ResizableArray<int>& negCounts, ResizableArray<string>& reviews, ResizableArray<int>& userRatings, int totalWords, Ms timeElapsed) {
    string input;
    int choice = -1;  // Initialize with an invalid value
    int totalReviews = reviews.getSize();
    int positiveWordCount = posCounts.getSize();
    int negativeWordCount = negCounts.getSize();

    do {
        cout << "Main Menu:\n";
        cout << "1 - Display Positive Words\n";
        cout << "2 - Display Negative Words\n";
        cout << "3 - Generate Sentiment Analysis\n";
        cout << "4 - Print Summary\n";
        cout << "0 - Exit\n";
        cout << ">> ";
        cin >> input;

        // Try to convert input to an integer
        try {
            choice = stoi(input);  // Convert string input to integer
        }
        catch (invalid_argument&) {
            cout << "Invalid input. Please enter a valid number.\n";
            continue;  // Skip the rest of the loop and ask for input again
        }

        switch (choice) {
        case 1:
            displayWordsWithOccurrences(positiveWords, posCounts);
            break;
        case 2:
            displayWordsWithOccurrences(negativeWords, negCounts);
            break;
        case 3:
        {
            clearScreen();
            string reviewInput;
            int reviewNumber;

            do {
                cout << "Enter review number to analyze (Q to exit): ";
                cin >> reviewInput;

                if (reviewInput == "Q" || reviewInput == "q") {
                    clearScreen();  // Clear screen before returning to the main menu
                    break;  // Exit back to main menu
                }

                try {
                    reviewNumber = stoi(reviewInput);  // Convert input to review number

                    if (reviewNumber > 0 && reviewNumber <= reviews.getSize()) {
                        generateSentimentAnalysis(reviews, userRatings, positiveWords, negativeWords, reviewNumber);
                        cout << "\n";
                    }
                    else {
                        cout << "Invalid review number. Please enter a number between 1 and " << reviews.getSize() << ".\n";
                    }
                }
                catch (invalid_argument&) {
                    cout << "Invalid input. Please enter a valid number or Q to exit.\n";
                }

            } while (true);

            break;  // Return to the main menu after exiting the loop
        }
        case 4:
            displaySummary(totalReviews, totalWords, positiveWordCount, negativeWordCount, timeElapsed);
            cout << "\n";
            break;
        case 0:
            break;
        default:
            cout << "Invalid choice. Try again.\n";  // For numbers that are outside of valid choices
        }
    } while (choice != 0);
}


int main() {
    try {
        ResizableArray<string> positiveWords;
        ResizableArray<string> negativeWords;
        ResizableArray<string> reviews;
        ResizableArray<int> posCounts;
        ResizableArray<int> negCounts;
        ResizableArray<int> userRatings;

        // Load words from text files
        loadWordsFromFile("positive-words.txt", positiveWords);
        loadWordsFromFile("negative-words.txt", negativeWords);

        // Sort the word lists
        positiveWords.insertionSort();
        negativeWords.insertionSort();

        // Load reviews and ratings from CSV
        int totalWords = 0;
        loadReviewsAndRatingsFromCSV("tripadvisor_hotel_reviews.csv", reviews, userRatings, totalWords);

        // Ask the user if they want to perform binary search
        cout << "Do you want to perform Binary Search? Yes - 1, No - 0\n>> ";
        int performBinarySearch;
        cin >> performBinarySearch;

        if (performBinarySearch == 1) {
            clearScreen();  // Clear the console before starting the analysis

            // Start timer
            Timer::time_point start = Timer::now();

            // Perform Semantic analysis using binary search and print summary
            performSemanticAnalysis(reviews, positiveWords, posCounts, negativeWords, negCounts, totalWords, start);

            // Stop timer
            Timer::time_point stop = Timer::now();
            Ms duration = std::chrono::duration_cast<Ms>(stop - start);

            // Display the results
            int totalReviews = reviews.getSize();
            int positiveWordCount = posCounts.getSize();
            int negativeWordCount = negCounts.getSize();
            displaySummary(totalReviews, totalWords, positiveWordCount, negativeWordCount, duration);
            cout << "\n";

            displayMenu(positiveWords, posCounts, negativeWords, negCounts, reviews, userRatings, totalWords, duration);
        }

        else {
            cout << "Exiting without performing binary search.\n";
        }

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
