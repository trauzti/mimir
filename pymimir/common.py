from math import log

class Entry:
    def __init__(self, key, value=None):
        self.key = key
        self.RUbit = 1
        self.value = value
        self.activity = None

class statbase:
    def __init__(self):
        self.hits = 0
        self.misses = 0
        self.requests = 0

    def __repr__(self):
        return "DUMMYstats"

    def Set(self, e, l):
        pass

    def Del(self, e):
        pass

    ### The clock hand touched the element
    ### Required by the IBM algorithm
    def Touch(self, e):
        pass

    def Miss(self, l):
        self.misses += 1
        self.requests += 1

    def Hit(self, e, l):
        self.hits += 1
        self.requests += 1

    def make_pdf(self):
        pass

    def sanitycheck(self, l):
        pass

class ghostbase:
    def __init__(self, capacity=0, filepath="ghostlist.bf"):
        pass
    def Miss(self, key):
        pass
    def Evict(self, key):
        pass
    def make_pdf(self):
        pass
    def printStatistics(self):
        pass

def make_cdf(pdf, numelements, divisor):
    cdf = [0.0] # means cache of size 0
    divisor = float(divisor)
    sm = 0.0
    assert divisor >= 1
    for i in xrange(numelements):
        sm += pdf[i]
        cdf.append(sm / divisor)
    return cdf

def normalize_array(A):
    sm = float(sum(A))
    return [x / sm for x in A]


def KLdiv(PP, QQ):
    sm = 0.0
    assert len(PP) == len(QQ)
    P = normalize_array(PP)
    Q = normalize_array(QQ)
    assert abs(sum(P) -1) < 1.0e-9
    assert abs(sum(Q) -1) < 1.0e-9
    # log(x) : the natural logarithm (base e) of x
    for i in xrange(len(P)):
        if P[i] > 0.0:
            sm += log(P[i]) * P[i]
        if Q[i] > 0.0:
            sm -= log(Q[i]) * P[i]
            #sm += (log(P[i] / float(Q[i])) * P[i])
        #else:
        #    print "Skipping (%d,%d) in KLdiv" % (#P[i], Q[i])
    #from ipdb import set_trace; set_trace()
    assert sm >= 0
    return sm


# Takes in two CDFs and returns the maximum distance between those two
# Does not normalize the arrays!
def KSdist(P, Q):
    assert len(P) == len(Q)
    return max( [ abs(P[i]-Q[i]) for i in xrange(len(P)) ] )


### The Mean Absolute Error
### Doesn't normalize with the maximum value!
def MAE(P, Q):
    assert len(P) == len(Q)
    sm = 0.0
    for i in xrange(len(P)):
        sm += abs(P[i] - Q[i])
    sm = sm / float(len(P))
    return sm

### The Mean Relative Error
### Divide with the true array
def MRE(P, Q):
    assert len(P) == len(Q)
    sm = 0.0
    for i in xrange(len(P)):
        if P[i] > 0.0:
            sm += abs(P[i] - Q[i]) / P[i]
    sm = sm / float(len(P))
    return sm

if __name__ == "__main__":
    #assert KLdiv(,P) < 1.0e-10
    #assert MSE(P,P) < 1.0e-10
    pass
