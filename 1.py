N=int(input("Enter a no:"))
c=0
for i in range(1,N+1):
    c+=str(i).count('0')
print(c)