all:frontend backend
clean:
	rm frontend backend

frontend:frontend.c
	gcc frontend.c -o frontend
backend:backend.c
	gcc backend.c -o backend