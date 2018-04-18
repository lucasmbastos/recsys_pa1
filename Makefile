all:
	g++ -std=c++11 -Wall -O3 -g3 -o recommender main.cpp

clean:
	rm recommender

run: all
	./recommender ratings.csv targets.csv > submission.csv