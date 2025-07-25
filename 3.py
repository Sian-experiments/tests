'''#tastiness core
n=int(input())
k=int(input())
large=0

a=list(map(int,input().split()))
s=sum(a[0:k])
for i in range(n-k):
    if s>large:
        large=s
    s+=a[k+i]-a[i]
print(large)

#count subarr
n=int(input())
k=int(input())
z=int(input())
c=0
a=list(map(int,input().split()))
s=sum(a[0:k])
for i in range(n-k):
    if s==z:
        c+=1
    s+=a[k+i]-a[i]
print(c)

#wrong but correct answer
a=list(map(int,input().split()))
s=0
large=a[0]
for i in range(len(a)):
    for j in range(len(a)):
        s=sum(a[i:j+1])
        if s>large:
            large=s
print(large)

#correct
a=list(map(int,input().split()))
s=0
large=a[0]
for i in range(len(a)):
    s=0
    for j in range(i,len(a)):
        s+=a[j]
        if s>large:
            large=s
print(large)

#optimised
a=list(map(int,input().split()))
large=a[0]
s=0
for i in range(len(a)):
    s+=a[i]
    if s<0:
        s=0
    if s>large:
        large=s
print(large)
#subarray print
a=list(map(int,input().split()))
large=a[0]
s=0
si=0
ei=0
for i in range(len(a)):
    s+=a[i]
    if s<0:
        s=0
        si=ei=i+1
    if s>large:
        large=s
        maxsi=si
        maxei=ei
    ei=ei+1
print(a[maxsi:maxei])

#jackfruit
n,d=map(int,input().split())
s=0
k=0
a=list(map(int,input().split()))
for j in range(n-1):
    for i in range(n-j-1):
        if a[i]>a[i+1]:
            a[i],a[i+1]=a[i+1],a[i]
for i in a[::-1]:
    if k!=d:
        s+=i
        k+=1
print(s)

#chinese
n=int(input())
d=list(map(int,input().split()))
ni=list(map(int,input().split()))
x=int(input())
l=[]
d.sort()
ni.sort()
for i in range(-1,-len(d),-1):
    m=1000
    for j in range(len(ni)):
        s=d[i]+ni[j]
        if s<m:
            m=s
            ni[i]=1000
    l.append(m)
print(l)
for i in l:
    if i>x:
        s+=(x-i)
print(100*s)
'''
n=int(input())
v1=list(map(int,input().split()))
v2=list(map(int,input().split()))
s=0
k=0
def sortkaro(a):
    for j in range(n-1):
        for i in range(n-j-1):
            if a[i]>a[i+1]:
                a[i],a[i+1]=a[i+1],a[i]
sortkaro(v1)
sortkaro(v2)
for i in range(n):
    p=v1[i]*v2[-1-k]
    s+=p
    k+=1
print(s)