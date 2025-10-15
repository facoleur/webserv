### Parsing implementation example
#### [CSE12 UCSD - Abstract Syntax Trees](https://cseweb.ucsd.edu/~kube/cls/12.s13/Lectures/lec16/lec16.pdf)
- Non-terminal symbol `<A>` defined as `<A> := <B> | <C>`:
```code
public static A parse (String s)
{
	if (B.parse(s) != null)
	{
		// the string satisfies definition of <B>; so,
		// make a new A node, with the result of B.parse(s)
		// as its child, and return it
	}
	else if (C.parse(s) != null)
	{
		// the string satisfies definition of <C>; so,
		// make a new A node, with the result of C.parse(s)
		// as its child, and return it
	}
	else // the string does not satisfy definition of <A>
		return null;
}
```