
(define (problem emptyroom-2)
 (:domain emptyroom)
 (:objects
   p1    p2  p3 )
(:init 
(x-above p2 p1) (x-above p3 p2)
(oneof  (x-pos p3)  (x-pos p2)  )
(unknown (x-pos p2)) 
(unknown (x-pos p3)) 


) 

 (:goal
      (x-pos p1))
 )

