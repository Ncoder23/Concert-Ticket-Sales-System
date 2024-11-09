# Concert-Ticket-Sales-System

## Project Overview
This project simulates a ticket sales system for a concert, ensuring concurrent handling of customer requests while maintaining synchronization and avoiding conflicts in seat assignments. The project utilizes multithreading and synchronization mechanisms to simulate real-world scenarios effectively.

---

## Features
1. **Clock Tick Simulation**  
   - The main thread generates clock ticks to represent time progression.
   - Each tick represents one minute, and all operations are synchronized accordingly.

2. **Thread States**  
   - Seller threads transition between the following states:
     - **Waiting:** Waiting for a new customer.
     - **Serving:** Handling a customer from the queue.
     - **Processing:** Performing sale-related computations.
     - **Completing:** Finalizing the sale for the customer.

3. **Seat Assignment**  
   - A 2D matrix simulates concert seating.
   - Ensures only one thread modifies the matrix at a time to prevent conflicts.

4. **Random Delays**  
   - Simulates real-world delays in sales processing based on random time intervals.

5. **Concurrency and Synchronization**  
   - All ten seller threads handle client requests concurrently while serializing seat assignments.

6. **Termination**  
   - The simulation ends when all seats are sold or the allotted simulation time is reached.

---

## Workflow
1. **Initialization**  
   - Set up the concert seat matrix, lock mutex, customer queue for each seller, and threads.

2. **Thread Management**  
   - Initialize seller threads and set them to wait for clock ticks.

3. **Clock Tick Simulation**  
   - Main thread sends clock ticks and ensures synchronization among threads.

4. **Customer Handling**  
   - Sellers process customer requests, simulate delays, and complete sales.

5. **Termination**  
   - Sellers stop when the concert is sold out or simulation time runs out.

---

## Key Assumptions
- Each clock tick represents one minute.
- Sellers handle one customer at a time.
- All threads must complete their tasks for a time quanta before the next clock tick is generated.
- Synchronization mechanisms ensure no conflicts occur during seat assignments.

---

## Results
The simulation demonstrates that:
- Concurrent handling of up to 10 seller threads is possible.
- Seat assignments are serialized, preventing conflicts.
- The output shows multiple clients being served concurrently (up to 8 at a time).

---

## Getting Started
### Prerequisites
- C++ compiler
- Understanding of threading and synchronization concepts

### How to Run
1. Clone the repository:
   ```bash
   git clone https://github.com/Ncoder23/Concert-Ticket-Sales-System.git
2. Navigate to the project directory:
    ```bash
    cd Concert-Ticket-Sales-System
    ```
3. Compile the code:
    ```bash
    g++ -pthread -o simulation main.cpp
    ```
4. Run the simulation:
    ```
    ./simulation
    ```