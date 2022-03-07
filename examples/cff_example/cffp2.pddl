
(define (problem emptyroom-2)
 (:domain emptyroom)
 (:objects
   p1    p2 - room )
(:init 
(x-above p2 p1) 
(oneof  (x-pos p1)  (x-pos p2)  )
(unknown (x-pos p1)) 
(unknown (x-pos p2)) 
;; (unknown (x-pos p3)) 

) 

 (:goal
      (x-pos p2))
 )

