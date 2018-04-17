#include <iostream>
#include <map>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <set>


std::map<int, double> user_sum;
std::map<int, double> user_total;

std::map<int, double> item_sum;
std::map<int, double> item_total;

std::map<int, double> user_mean;
std::map<int, double> item_mean;

std::map<int, double> cosine_norm;

std::map<int, std::set<int>> user_per_items;
std::map<int, std::set<int>> items_rated_by_user;


std::set<int> all_items;
std::vector<int> all_items_vector;

double all_item_mean = 0;

//https://stackoverflow.com/a/1489928
int GetNumberOfDigits (int i)
{
    return i > 0 ? (int) log10 ((double) i) + 1 : 1;
}



// M[Item][User] = Rating
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

int trim_id(std::string str) {

    std::string output; // From StackOverflow -- https://stackoverflow.com/q/20326356
    output.reserve(str.size()-1);
    for(size_t i = 1; i < str.size(); ++i)
        output += str[i];

    return std::stoi(output);
}

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

        // User Mean
        if(!user_sum.count(user_id)) {
            user_sum[user_id] = 0;
            user_total[user_id] = 0;
        }

        user_sum[user_id] += rating;
        user_total[user_id]+= 1;

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
//            std::cout << matrix[item_key][user_key] << " - " << user_mean[user_key] << " - " << item_mean[item_key] << std::endl;
            matrix[item_key][user_key] = matrix[item_key][user_key] - user_mean[user_key] - item_mean[item_key];

//            std::cout << matrix[item_key][user_key] << std::endl;

            cosine_norm[item_key] += pow(matrix[item_key][user_key],2);
        }
    }
}

void calculate_cosine_norm() {
    for(const int &cosine_key: all_items_vector)
        cosine_norm[cosine_key] = sqrt(cosine_norm[cosine_key]);
}

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

    if(!user_total.count(user_id) && all_items.count(item_id))
        return item_mean[item_id];

    if(!user_total.count(user_id) && !all_items.count(item_id))
        return all_item_mean;

    if(user_total.count(user_id) && !all_items.count(item_id))
        return user_mean[user_id];

    std::vector<int> all_items_rated_by_user(items_rated_by_user[user_id].begin(), items_rated_by_user[user_id].end());
//    std::vector<int> all_users_that_rated_the_item(user_per_items[item_id].begin(), user_per_items[item_id].end());

    double total_similarity = 0;
    double total_rate = 0;


    for(int item_rated_by_user : all_items_rated_by_user) {


        double similarity = 0;

        std::set<int> intersect;
        set_intersection(user_per_items[item_id].begin(),user_per_items[item_id].end(),user_per_items[item_rated_by_user].begin(),user_per_items[item_rated_by_user].end(),
                         std::inserter(intersect,intersect.begin()));

        for(int user_that_rated_the_item: intersect) {
//            std::cout << matrix[item_id][user_that_rated_the_item] << " - " << matrix[item_rated_by_user][user_that_rated_the_item] << std::endl;
            similarity += matrix[item_id][user_that_rated_the_item] * matrix[item_rated_by_user][user_that_rated_the_item];

        }

        similarity = similarity/(cosine_norm[item_id]*cosine_norm[item_rated_by_user]);

        total_similarity+= similarity;
        total_rate += (matrix[item_rated_by_user][user_id]) * similarity;


    }


//    std::cout << total_rate << " - " << total_similarity << std::endl;

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

int main() {


    RatingMatrix rating_matrix_item_first = read_from_file("/home/lucas/CLionProjects/recsys_tp1/ratings.csv");

    compute_item_mean();
    compute_user_mean();

    normalize_matrix_item(rating_matrix_item_first);

    calculate_cosine_norm();



    read_targets("/home/lucas/CLionProjects/recsys_tp1/targets.csv", rating_matrix_item_first);

}