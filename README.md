## CPPLOB
### The limit order book / matching engine implementation for the Bristol Inter-University Stock Exchange Simulator (BISES)

Operation Complexities:
- Adding: O(logn) for first order at a price, O(1) afterwards
- Cancelling: O(1)
- Matching: O(1)
- Updating: O(1)

#### Benchmarks:

Benchmarks were taken on a i7-1360p @ 3.7 GHz with 32gb of RAM

- Simple program adding 100 Million Bids then 100 Million Asks at price 1, with quantity 1:
> 6.6-10.0 seconds taken, ~30-45 million operations processed per second (Bids/Asks/Matches)
