#ifndef INTERVAL_HPP
#define INTERVAL_HPP

#include <algorithm>
#include <iostream>
#include <memory>

template<class Time>
class Interval {
    //For now just descrite time considered

private:
    Time start, end;
public:
    Interval(Time a, Time b) {
        if (a > b) {
            this->start = b;
            this->end = a;
        } else {
            this->start = a;
            this->end = b;
        }
    }

    Interval() {
    }

    Interval(const std::pair<Time, Time> p)
            : Interval(p.first, p.second) {
    }

    Interval(const Interval<Time> &orig)
            : start(orig.start), end(orig.end) {
    }

    const Time from() const {
        return start;
    }

    const Time until() const {
        return end;
    }

    const Time &upto() const
    {
        return end;
    }

    const Time &min() const {
        return start;
    }

    const Time &max() const {
        return end;
    }

    void setMin(Time a){
        start = a;
    }

    void setMax(Time a){
        end = a;
    }

    bool contains(Interval<Time> &other) {

        return from() <= other.from() && other.until() <= until();
    }

    bool contains(const Time &point) const {
        return from() <= point && point <= until();
    }

    bool disjoint(const Interval<Time> &other) const {
        return other.until() < from() || until() < other.from();
    }

    bool intersects(const Interval<Time> &other) const {
        return not disjoint(other);
    }

    Time length() const {
        return until() - from();
    }

    void widen(const Interval<Time> &other) {
        start = std::min(from(), other.from());
        end = std::max(until(), other.until());
    }

    void lowerBound(Time lb) {
        start = std::max(lb, start);
    }

    void extendTo(Time b_at_least) {
        end = std::max(b_at_least, end);
    }

    Interval<Time> merge(const Interval<Time> &other) const {
        return Interval<Time>{std::min(from(), other.from()),
                              std::max(until(), other.until())};
    }

    bool operator==(const Interval<Time> &other) const {
        return other.from() == from() && other.until() == until();
    }

    void operator+=(Time offset) {
        start += offset;
        end += offset;
    }

    void operator-=(Time offset) {
        start -= offset;
        end -= offset;
    }

    Interval<Time> operator+(const Interval<Time> &other) const {
        return {start + other.from(), end + other.until()};
    }

    Interval<Time> operator+(const std::pair<Time, Time> &other) const {
        return {start + other.first, end + other.second};
    }

    Interval<Time> operator-(Time offset) {
        return {start - offset, end - offset};
    }

    Interval<Time> operator+(Time offset) {
        return {start + offset, end + offset};
    }

    Interval<Time> operator-(const Interval<Time> &other) const {
        return {start - other.from(), end - other.until()};
    }

    Interval<Time> operator|(const Interval<Time> &other) const {
        return merge(other);
    }

    void operator|=(const Interval<Time> &other) {
        widen(other);
    }

    friend std::ostream& operator<< (std::ostream& stream,
                                     const Interval<Time>& i){
        stream << "I";
        stream << "[" << i.from() << ", " << i.until() << "] ";
        return stream;
    }

    friend std::string to_string(const Interval<Time>& i){
        std::string s = "I";
        s += "[" + std::to_string(i.from()) + ", " + std::to_string(i.until()) + "] ";
        return s;
    }

};

template<class T, class X, Interval<T> (*map)(const X &)>
class IntervalLookupTable {
    typedef std::vector<std::reference_wrapper<const X>> Bucket;

private:

    std::unique_ptr<Bucket[]> buckets;
    const Interval<T> range;
    const T width;
    const unsigned int num_buckets;


public:

    std::size_t bucket_of(const T &point) const {
        if (range.contains(point)) {
            return static_cast<std::size_t>((point - range.from()) / width);
        } else if (point < range.from()) {
            return 0;
        } else
            return num_buckets - 1;
    }

    IntervalLookupTable(const Interval<T> &range, T bucket_width)
            : range(range), width(std::max(bucket_width, static_cast<T>(1))), num_buckets(1 + std::max(
            static_cast<std::size_t>(range.length() / this->width),
            static_cast<std::size_t>(1))) {
        buckets = std::make_unique<Bucket[]>(num_buckets);
    }

    void insert(const X &x) {
        Interval<T> w = map(x);
        auto a = bucket_of(w.from()), b = bucket_of(w.until());
        assert(a < num_buckets);
        assert(b < num_buckets);
        for (auto i = a; i <= b; i++)
            buckets[i].push_back(x);
    }

    const Bucket &lookup(T point) const {
        return buckets[bucket_of(point)];
    }

    const Bucket &bucket(std::size_t i) const {
        return buckets[i];
    }

};

#endif



