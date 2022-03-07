
(define (domain emptyroom)

  (:predicates 	(x-pos ?pos ) 
                (x-above ?phigh ?plow )
    )

  (:action right
   :parameters ()
   :effect (and 
    (forall (?phigh ?plow)
                     (when (and (x-above ?phigh ?plow)
                                (x-pos ?plow))
                           (and (x-pos ?phigh) (not (x-pos ?plow))))))
  )
  (:action left
   :parameters ()
   :effect (and 
      (forall (?phigh ?plow)
                   (when (and (x-above ?phigh ?plow)
                                (x-pos ?phigh))
                           (and (x-pos ?plow) (not (x-pos ?phigh))))))
  )
)
