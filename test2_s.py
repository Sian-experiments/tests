'''l=list(map(input().split(" ")))
j=0
for i in l:
    if i.split() in l[j::]:
        print(True)

l1=[]
n=int(input())
l=input().split(" ")
print(l)
for i in l:
    l1.append(i.sort())
for i in l:

n=int(input())
s=input()
for i in s:
    if i.isnumeric() and s.count(i)==1:
        print(i)
        break

[sumof set*k times- sum(arr)]//k-1

k=int(input())
l=list(map(int,input().split(" ")))
setarr=set(l)
print((sum(setarr)*k-sum(l))//(k-1))

n=int(input())
l=list(map(int,input().split(" ")))
l.sort()
k=int(input())
r=[]
for i in range(n):
    for j in l[i::]:
        if abs(l[i]-j)==k and l[i] not in r and j not in r:
            r.append(l[i])
            r.append(j)
print(len(r)//2)

#missing no
n=int(input())
l=list(map(int,input().split(" ")))
s1=n*(n+1)//2
s2=sum(l)
print(s1-s2)
'''





