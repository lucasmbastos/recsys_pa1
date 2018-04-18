/*

MIT License

Copyright (c) 2018 Lucas de Miranda Bastos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <set>


// This is the code used to solve RecSys Practical Assignment 1, the code has lots of bad smells
// like global varibales. Moreover, all content of work is in this file. The reason why this
// problems exist is related with bad time management of author.


/**

@Author: Lucas de Miranda Bastos

**/

/*
* Observations:
*
* 1 - Both the items and the users have the IDs converted to integger (to improve efficiency), so
* the key of majority of maps are integers.
*
*/

//--- Global Variables

// Handle user mean 
std::map<int, double> user_sum;
std::map<int, double> user_total;
std::map<int, double> user_mean;

// Handle item mean
std::map<int, double> item_sum;
std::map<int, double> item_total;
std::map<int, double> item_mean;
double all_item_mean = 0; // Stores the mean of product means

std::map<int, double> cosine_norm; // Used to calculate cosine similarity

// Tracks all users that rate the items and vice-versa.
std::map<int, std::set<int>> user_per_items;
std::map<int, std::set<int>> items_rated_by_user;

// Set and vector with all items
std::set<int> all_items;
std::vector<int> all_items_vector;


//https://stackoverflow.com/a/1489928
int GetNumberOfDigits (int i)
{
    return i > 0 ? (int) log10 ((double) i) + 1 : 1;
}



// M[Item][User] = Rating --- This is how the RatingMatrix is stored
typedef std::map<int, std::map<int, double>> RatingMatrix;

void addToMatrix(int item, int user, double rating, RatingMatrix &matrix) {

    if(!matrix.count(item))
        matrix[item] = std::map<int,double>();

    matrix[item][user] = rating;

}

// https://stackoverflow.com/a/37454181
std::vector<std::string> split(const std::string& str, const std::string& delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    return tokens;
}

// Removes the prefix of ids and convert it to integger
int trim_id(std::string str) {

    std::string output; // From StackOverflow -- https://stackoverflow.com/q/20326356
    output.reserve(str.size()-1);
    for(size_t i = 1; i < str.size(); ++i)
        output += str[i];

    return std::stoi(output);
}

// This function reads the ratings data and extract all information that will be used. It still pre-
// computate lots of variables (e.g: used for mean) to save time.

RatingMatrix read_from_file(std::string file_path) {

    RatingMatrix output;

    std::ifstream train_data(file_path);

    std::string line;
    // Ignores first line
    std::getline(train_data, line);

    while(std::getline(train_data, line)) {
        int user_id;
        int item_id;
        double rating;

        std::vector<std::string> useritem_rating = split(line, ",");
        std::vector<std::string> user_item = split(useritem_rating[0], ":");

        user_id = trim_id(user_item[0]);
        item_id = trim_id(user_item[1]);
        rating = std::stod(useritem_rating[1]);

        // Handle the first occurence of user
        if(!user_sum.count(user_id)) {
            user_sum[user_id] = 0;
            user_total[user_id] = 0;
        }

        user_sum[user_id] += rating;
        user_total[user_id]+= 1;

        // // Handle the first occurence of item
        if(!all_items.count(item_id)) {
            item_sum[item_id] = 0;
            item_total[item_id] = 0;

            cosine_norm[item_id] = 0;

            all_items.insert(item_id);
        }


        item_sum[item_id] += rating;
        item_total[item_id]+= 1;


        user_per_items[item_id].insert(user_id);
        items_rated_by_user[user_id].insert(item_id);


        addToMatrix(item_id, user_id, rating, output);

    }

    train_data.close();

    for(auto it = all_items.begin(); it != all_items.end(); ++it)
        all_items_vector.push_back(*it);

    return output;

}

void compute_item_mean() {
    for(int key: all_items_vector) {
        item_mean[key] = item_sum[key] / item_total[key];
        all_item_mean+=item_mean[key];
    }

    all_item_mean = all_item_mean/all_items_vector.size();
}

void compute_user_mean() {
    std::vector<int> v;
    for(auto it = user_sum.begin(); it != user_sum.end(); ++it)
        v.push_back(it->first);

    for(int key: v)
        user_mean[key] = user_sum[key]/user_total[key];
}

void normalize_matrix_item(RatingMatrix &matrix) {

    for(int item_key: all_items_vector) {

        std::vector<int> v_user(user_per_items[item_key].begin(), user_per_items[item_key].end());

        for(int user_key: v_user) {
            matrix[item_key][user_key] = matrix[item_key][user_key] - user_mean[user_key] - item_mean[item_key];
            cosine_norm[item_key] += pow(matrix[item_key][user_key],2);
        }
    }
}

void calculate_cosine_norm() {
    for(const int &cosine_key: all_items_vector)
        cosine_norm[cosine_key] = sqrt(cosine_norm[cosine_key]);
}


// This function formats the output data in order to match the requirements of Kaggle.
void print_answer(int user_id, int item_id, double prediction) {

    int user_tab_size = 7-GetNumberOfDigits(user_id);
    int item_tab_size = 7-GetNumberOfDigits(item_id);

    std::string user_tab;
    std::string item_tab;

    user_tab = "u";
    item_tab = "i";

    user_tab = user_tab.append(user_tab_size, '0');
    item_tab = item_tab.append(item_tab_size, '0');

    std::cout << user_tab << user_id << ":" << item_tab << item_id << "," << prediction << std::endl;
}

double make_prediction(int user_id, int item_id, RatingMatrix &matrix) {

    // If user doesn't exist in training data
    if(!user_total.count(user_id) && all_items.count(item_id))
        return item_mean[item_id];

    // If both user and item doesn't exist in training data - Non-personalized Recomendation
    if(!user_total.count(user_id) && !all_items.count(item_id))
        return all_item_mean;

    // If item doesn't exist in training data
    if(user_total.count(user_id) && !all_items.count(item_id))
        return user_mean[user_id];


    std::vector<int> all_items_rated_by_user(items_rated_by_user[user_id].begin(), items_rated_by_user[user_id].end());

    double total_similarity = 0;
    double total_rate = 0;


    for(int item_rated_by_user : all_items_rated_by_user) {


        double similarity = 0;

        std::set<int> intersect;
        set_intersection(
            user_per_items[item_id].begin(),
            user_per_items[item_id].end(),
            user_per_items[item_rated_by_user].begin(),
            user_per_items[item_rated_by_user].end(),
            std::inserter(intersect,intersect.begin())
        );

        for(int user_that_rated_the_item: intersect) 
            similarity += matrix[item_id][user_that_rated_the_item] * matrix[item_rated_by_user][user_that_rated_the_item];


        similarity = similarity/(cosine_norm[item_id]*cosine_norm[item_rated_by_user]);

        total_similarity+= similarity;
        total_rate += (matrix[item_rated_by_user][user_id]) * similarity;


    }
    double prediction = (total_similarity>0) ? (total_rate/total_similarity) : 0;

    prediction = prediction  + item_mean[item_id] + user_mean[user_id];

    return prediction;


}

void read_targets(std::string file_path, RatingMatrix &matrix) {

    std::cout << "UserId:ItemId,Prediction" << std::endl;

    std::ifstream target_data(file_path);

    std::string line;
    // Ignores first line
    std::getline(target_data, line);

    std::vector<int> user_ids;
    std::vector<int> item_ids;
    std::vector<double> predictions;

    while(std::getline(target_data, line)) {
        int user_id;
        int item_id;

        std::vector<std::string> user_item = split(line, ":");

        user_id = trim_id(user_item[0]);
        item_id = trim_id(user_item[1]);

        user_ids.push_back(user_id);
        item_ids.push_back(item_id);
    }

    target_data.close();

    for(unsigned int i=0;i<user_ids.size(); i++) {

        double prediction = make_prediction(user_ids[i], item_ids[i], matrix);
        predictions.push_back(prediction);
        print_answer(user_ids[i], item_ids[i], prediction);
    }
}

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cout << "Invalid number of arguments" << std::endl;
        return 1;
    }


    RatingMatrix rating_matrix_item_first = read_from_file(argv[1]);

    compute_item_mean();
    compute_user_mean();

    normalize_matrix_item(rating_matrix_item_first);

    calculate_cosine_norm();



    read_targets(argv[2], rating_matrix_item_first);

}
