(define (problem empty-room-3-rooms-2-and-3-unknown)
	(:domain emptyroom)
	(:objects
		p1 p2 p3 
	)
	(:init
		(x-above p3 p2) 
		(x-above p2 p1)
		(unknown (x-pos p2))
		(unknown (x-pos p3))
		(oneof (x-pos p2) (x-pos p3))
	)
	(:goal
		(x-pos	p1)
	)
)
