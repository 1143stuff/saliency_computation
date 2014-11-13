

all:saliencyOut_optimised saliencyOut_unoptimised 

saliencyOut_optimised:
	g++ saliencyOptimised.cpp -o2 -L/usr/local/include/opencv2 -lopencv_core -lopencv_imgproc -lopencv_highgui -pthread -o saliencyOut_optimised


saliencyOut_unoptimised:
	g++ saliencyUnoptimised.cpp -o2 -L/usr/local/include/opencv2 -lopencv_core -lopencv_imgproc -lopencv_highgui -pthread -o saliencyOut_unoptimised






clean:
	rm -rf *o saliencyOut_optimised  saliencyOut_unoptimised
	

