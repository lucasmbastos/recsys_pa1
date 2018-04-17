cmake_minimum_required(VERSION 3.10)
project(recsys_tp1)

set(CMAKE_CXX_STANDARD 11)

add_executable(recsys_tp1 main.cpp MathUtils.cpp MathUtils.h SimilarityItem.cpp SimilarityItem.h SimilarityCollection.cpp SimilarityCollection.h)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")