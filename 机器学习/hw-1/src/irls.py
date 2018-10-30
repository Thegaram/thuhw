import numpy as np
from sklearn.utils import Memory
from sklearn.datasets import load_svmlight_file

mem = Memory("./mycache")

training_dataset = "./a9a/a9a"
testing_dataset  = "./a9a/a9a.t"

eps = 10e-3
lamb = 0.03

@mem.cache
def get_data(path):
    data = load_svmlight_file(path)
    X = data[0].todense()
    y = np.reshape(data[1], (-1, 1)) # make y a proper vector
    y[y == -1] = 0 # make sure labels are {0, 1}
    return X, y

def w_init(d):
    return np.zeros((d, 1))

def p(y, X, w):
    N, d = X.shape
    assert w.shape == (d, 1)
    assert y == 0 or y == 1
    return (np.exp(y * np.dot(X, w))) / (1 + np.exp((np.dot(X, w))))

def pred(X, w):
    N, d = X.shape

    p_zero = p(0, X, w)
    p_one  = p(1, X, w)
    assert p_zero.shape == (N, 1)
    assert p_one.shape  == (N, 1)

    res = np.zeros((N, 1))
    res[p_zero >  p_one] = 0
    res[p_zero <= p_one] = 1
    assert res.shape == (N, 1)
    return res

def accuracy(X, y, w):
    N, d = X.shape
    assert y.shape == (N, 1)
    assert w.shape == (d, 1)

    preds = pred(X, w)
    assert preds.shape == (N, 1)
    return np.sum(preds == y) / N

def sigmoid(vec):
  return 1 / (1 + np.exp(-vec))

def mu(X, w):
    N, d = X.shape
    assert w.shape == (d, 1)
    res = sigmoid(np.dot(X, w))
    assert res.shape == (N, 1)
    return res

def L2_norm(w):
    return np.sum(np.dot(w.T, w))

def L(X, y, w):
    N, d = X.shape
    assert y.shape == (N, 1)
    assert w.shape == (d, 1)
    reg = -lamb/2 * L2_norm(w)
    return reg + np.sum(np.multiply(np.dot(X, w), y) - np.log(1 + np.exp(np.dot(X, w))))

def print_stats(i, X, y, Xt, yt, w, diff):
    l = L(X,y,w)
    a = accuracy(X, y, w)
    at = accuracy(Xt, yt, w)
    print("--------------")
    print(">> iteration #" + str(i))
    print("L = " + str(l))
    print("acc_train = {0:.4f}%".format(a * 100))
    print("acc_test = {0:.4f}%".format(at * 100))
    print("diff = " + str(diff))
    print("L2 norm = " + str(L2_norm(w)))
    print("--------------\n")

def get_update(X, y, w):
    N, d = X.shape
    assert y.shape == (N, 1)
    assert w.shape == (d, 1)

    m = mu(X, w)
    R = np.diagflat(np.multiply(m, (1 - m)))

    H = np.dot(np.dot(X.T, R), X)
    assert H.shape == (d, d)

    # reg term
    H = H + lamb * np.identity(d)
    assert H.shape == (d, d)

    H_inv = np.linalg.pinv(H)
    assert H_inv.shape == (d, d)

    m_y = np.subtract(m, y)
    assert m_y.shape == (N, 1)

    g = np.dot(m_y.T, X)
    assert g.shape == (1, d)

    # reg term
    g = g + lamb * w.T
    assert g.shape == (1, d)

    res = np.dot(H_inv, g.T)
    assert res.shape == (d, 1)
    return res

def train(X, y, Xt, yt):
    N, d = X.shape
    assert y.shape == (N, 1)
    M, _ = Xt.shape
    assert Xt.shape == (M, d)
    assert yt.shape == (M, 1)

    w = w_init(d)
    print_stats(0, X, y, Xt, yt, w, 0)

    iteration = 1
    diff = 10000.0
    while diff >= eps:
        w_diff = get_update(X, y, w)
        w -= w_diff

        diff = np.linalg.norm(w_diff)
        print_stats(iteration, X, y, Xt, yt, w, diff)
        iteration += 1

def main():
    X,  y  = get_data(training_dataset)
    Xt, yt = get_data(testing_dataset)

    # the last dimension is not represented in the test data
    Xt = np.append(Xt, np.zeros((Xt.shape[0], 1)), axis=1)

    w = train(X, y, Xt, yt)

if __name__ == "__main__":
    main()
