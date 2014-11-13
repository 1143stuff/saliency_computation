SALIENCY MAP ON INTEL GALILEO GEN2
----------------------------------


we have two src(.cpp) files saliencyUnoptimised and saliencyOptimised(functional changes)

1.compile:
---------

saliencyOptmised : make saliencyOut_optimised

saliencyUnoptimised: make saliencyOut_unoptimised


2.run:
------

./saliencyOut_optimised <input image>

./saliencyOut_unoptimised <input image>


3.output:
---------

saliencyOut_unoptimised.bmp

saliencyOut_optimised.bmp



4.viewing:
----------

do "scp" 

 


