Px = 850.5
Py = 695.5

Lsx = 1027.28271
Lsy = 434.700897
Lex = 836.817627
Ley = 715.686218

Rsx = 1027.28271
Rsy = 434.700897
Rex = 836.817627
Rey = 715.686218

#Ax*By - Ay*Bx
#A=Le-Ls; B=P-Ls
#(Ax)*(By) - (Ay)*(Bx)
W0 = (Lex-Lsx)*(Py-Lsy) - (Ley-Lsy)*(Px-Lsx)

#w0 = ((le - ls) ^ (vt - ls)).z
#w1 = ((vt - rs) ^ (re - rs)).z

W1 = (Px-Rsx)*(Rey-Rsy) - (Py-Rsy)*(Rex-Rsx)

print(W0)
print(W1)
