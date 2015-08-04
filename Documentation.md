The aim of this documentation is to conserve my internship experiences and serve as an informative source for someone 
who is just starting to learn the details of HPCC systems from scratch.

The overall goal of my intersnhip is to improve the efficiency of execution of child queries. What is a child query and how can we go about improving it is explained in later parts of the document.

#Before the internship
Since, I applied for this using Google Summer of Code, I had to develop a good idea about the project to develop a proposal. The first goal was to get an understanding of what HPCC system is and how does it work. The links on this page does this job pretty well:

    http://hpccsystems.com/community/training-videos
    
I went through all the links under these topics as they were very much related to my work. 
* HPCC
* ECL 
* Installing HPCC

In brief, HPCC is a platform to execute powerful queries on huge amounts of data (also called Big Data). The queries are written in an in-house declarative language called ECL. 

HPCC is a direct competitor of MapReduce. A major advantage that HPCC has over MapReduce is that its query language is much more powerful and allows a variety of complex operations. On the other hand, it is also a bit of a drawback, because the user needs to understand how to use ECL and its various tools which may be a bit of a learning curve.

Quite often I came across the terms: thor, hthor and roxie. I found it best to consider them as different physical clusters of computers. They vary in the way they execute certain aspects of an ECL query. It is my guess that thor is used for large datasets and roxie for small ones.

The next step was to understand some ECL queries and see how it worked. Each ECL query is first converted to xml and c++ code. The execution engine uses these two execute the query. Gavin sent me some basic example queries. In addition, the following two links helped me quite a lot:

https://github.com/hpcc-systems/HPCC-Platform/blob/master/ecl/eclcc/DOCUMENTATION.rst

http://hpccsystems.com/download/docs/ecl-language-reference/html

The first link is to understand some fine details in the processing of an ECL query. Now that I look back at it, a lot more things seem to make sense compared to how it looked when I initially read it. The second link is useful to understand any particular line in the ECL code. Since, there could be many syntax of any given operation, this may get confusing. It is good to think of them as overloaded functions. 

With this I found that the best way to identify a child query is to look for the element: child="1" in the generated xml graph. Via email, I was told that what are the potential problems in the generation of child queries. I tried to come up with ECL queries to create those problems but was unable to do so. Eventually, Gavin sent me a bunch of examples ecl codes: gsoc1 to gsoc6, which explored the relevant problems. The ecl files can be found here:

https://github.com/hpcc-systems/HPCC-Platform/tree/master/testing/regress/ecl

Before getting to know HPCC, I have worked on mySQL and Oracle. In one of my courses, we had to develop a new database from scrach based on some C++ code framework. Until then, my view of a database system limited to some classic algorthms to execute database operations like select, sort, join etc and how to write functional queries in sql to get these done. The internship helped me explore to some extent a system in which all these features are already there. My work was to make some small tweaks and hope to have a favorable effect at the end. 

#During the internship

The initial steps involved forking the HPCC system on Github, building it locally and then installing it. There were a few glitches in the process, for example, at one point the ECL watch went blank. But they were solved after by talking with Gavin and Gordon. Then I went through the documentation of github to understand the basics. I believe my experience with github would be very productive for me in the future. I sent the pull request to the repo with the six examples that Gavin sent me. 

Thereafter I setup the eclipse CDT and started debugging the code. It took some time to understand well the details of how to work with eclipse CDT. I have been used to working with emacs which had much limited features. After some discussions with Gavin, I was finally able to setup the debug configuration. It also took sometime to make sure that the eclcc.ini had all the required paths. 

I started with debugging gsoc5. The part of this ecl code which was the point of concern is this:
```
 outRecord t1(idRecord l) := TRANSFORM
     f := l.children(cid % 2 = 1); // those children whose cid % 2 = 1
     SELF.child1:= ROW(makeNested(f(cid % 3 != 1)));
     SELF.child2:= ROW(makeNested(f(cid % 4 != 1)));
     SELF := l;
 END;
 ```

The filter operation 'f' is evaluated inline twice for child1 and child2 as shown here:

		for (;vQ--;) {
			rowO = *curP++;
			if ((long long)*((unsigned long long *)(rowO + 0U)) % 2LL == 1LL) {
				if ((long long)*((unsigned long long *)(rowO + 0U)) % 3LL != 1LL) {
					crM.append(rowO);
				}
			}
		}
	...........
	...........
		for (;vU--;) {
			rowS = *curT++;
			if ((long long)*((unsigned long long *)(rowS + 0U)) % 2LL == 1LL) {
				if ((long long)*((unsigned long long *)(rowS + 0U)) % 4LL != 1LL) {
					crR.append(rowS);
				}
			}
		}

My first impression was that I should be just debugging looking at the variables and try to figure out the place where the filter activity is generated inline. Then, I can try to change the code to see if I can make it evaluate only once. But as it turned out things are much more complex than that. Gavin told me to look at canProcessInline function to see which operators are evaluate inline and which not. The operator in my interest for this case was no\_filter. But it occurred to me that the function is called for multiple operators and I could not relate many of these operators with the actual ecl code. Even for the operators while look like they are relavant to the ECL code, I could not understand the expression associated with it. I was then introduced to the logging functions like EclIR::dump\_ir() and DBGLOG(). This helped me a bit. But the dump of EclIR::dump\_ir(IHqlExpression) was not very clear about which operator is contained in it. It seemed that it had a lot of other information. Eventually, I found that there is another function: EclIR::getOperatorIRText() to print exactly which operator is it working on. I tried to dig a bit deeper in canProcessInline function and went a few levels in the code. After some discussion with Gavin, I came to following conclusions:

1. newAtom is used to indicate that the parent dataset or row that the column/fields is being selected from is not active. If a no_select has no newAtom attribute then it is a column selected from an active dataset (SQL cursor might be clearer).  E.g., ds(ds.myField = 10) ds.myField does not have a new atom because ds is not active within the scope it is used.
2. The following function is checking for a filter condition that doesn’t relate to the dataset being filtered.
```
bool filterIsTableInvariant(IHqlExpression * expr)
{
    IHqlExpression * dsSelector = expr->queryChild(0)->queryNormalizedSelector();
    ForEachChildFrom(i, expr, 1)
    {
        IHqlExpression * cur = expr->queryChild(i);
        if (containsSelector(cur, dsSelector))
            return false;
    }
    return true;
}
```
3. Sort operations are not executed inline. It generates a child query. The following function contains more information about it. 
```
ABoundActivity * HqlCppTranslator::doBuildActivitySort(BuildCtx & ctx, IHqlExpression * expr)
```
At this point I thought that the code is too complex to really understand every little details about it. 

I now shifted my focus on making my own dataset and maybe go on to experiment with how can I run queries on it to discover the problem with child queries. For this I wanted to get a good understanding about some basics of dabatabse creation in ECL like indexing, normalizing etc. I decided to go through this example ecl file for the purpose:
https://github.com/hpcc-systems/ecl-samples/blob/master/bundles/CellFormatter/GenData/GenData.ecl

Going through this raised a lot of questions in my mind. I will go through some of the points which I figures out after discussion with Gavin via email:

1. Certains terms are used interchangeably in the ecl documentation like field and column, recordset and dataset and table.
2. The normalize function:
```
NORMALIZE(BlankKids,TotalChildren,CreateKids(LEFT,COUNTER))
```
is an outdated idiom. It just seemed to call the CreateKids function TotalChildren (an integer) number of times on the dataset of BlankKids. The actual normalization process is done using other function, which I have not explored yet. A replacement for the above functions is:
```
DATASET(TotalChildren, CreateKids(COUNTER))
```
After this, I created my own dataset and is shown here:

The creation of the database was a bit of a hassle because there were lot of formatting like single quotes which I needed to correct in excel. I tried to create some examples next to figure out the problem of child query. I did come up with an example of a count query which could have been more efficient. But it looked like it will take me a long time to actually be able to master ECL and come up with examples which identifies the problems. I often felt lost in the immense number of operations to look at. I was told that I should concentrate on the operations which are assigned false by canProcessInline functions. But since I did not have full confidence that I understood the function properly, I could not feel like I could venture into making my own ECL queries. Also, I was still uncertain about the link between an ECL action and the node operator activity which is responsible to execute that action. 

Gavin was very understanding and he gave me an easier job of coming up with my own version of generated C++ code for gsoc5 and gsoc6 which would be efficient according to me. We were talking about making every activity in a child graph at this point. I went through the generated codes of gsoc5 and gsoc6 and came up with the following conclusions after discussion with Gavin:

1. There can be a node inside a graph and a graph inside a node. Nodes inside graphs are the activities within a graph.
2. Terms like cAc2 in c++ means activity in the node with id = 2 (note 2 at the end of cAc2) of the xml graph
3. The xml graph is generated using IHqlExpression class instance for the activites which return false from canProcessInline()  

I came up with a proposal of how should the generated C++ and xml files look like for gsoc5 and gsoc6 and can be seen here:


In summary, my idea was to store the result of every activity in a child graph and split the result to other child graphs who need it. My Reasoning for the approach: From what I gathered so far, most of the optimization are done in child graphs. So, why not every activity in a child graph. Then split its result accordingly to who needs it. 

## Code Generator
A major part of the internship was to understand the process of generating C++ and xml codes from an ECL query. The first step is converion of an ECL query into an expression graph. This expression graph consists of all the operations required to execute the query. Next, it is decided that which activity (or bunch of them) should be execute in a child query and which should be done inline. The function canProcessInline plays a major role in this decision. After this, some optimization are applied to the activities in the child graph. At the end the inline activities and the c++ implementation of childgraph activities are combined in c++ file. 

## Feedback
At the end of internship I have the following questions/suggestions:

1. What exactly is the structure of an xml graph. I see that there is a graph within a graph within a node and many other combinations. Some graph nodes look redundant and appear just as another rectangle in the graph representation. 
2. I think I was not able to use graph control in firefox. Though it was generating a graph it was not the same as Jamie saw in his own instance. Is there some instuction on how to use it with firefox?
2. I saw that some child queries have another child query inside it. Is that usual and is there any advantage of having that?
3. I do not have a complete understanding of Owned, Link and setClear etc. I think this needs a proper documentation with good examples.
4. I was caught in the error: Graph context not found. Is there a good way to print the value of the variable ctx?
5. I have quite a few confusions when it comes to writing ECL queries. For example, I am not sure what does ROW operation do. In the documentation it says it creates a single data record. Does it mean it creates a single record with one column? Or something else?
6. I had some trouble understaing the NORMALIZE operation. It seemed that all it did is perform certain transfrom operation n number of times. If there is a normalize operation which does the traditional normalization of tables, what kind of normalization it does (2nf, 3nf)?
6. When I dump an Ihqlexpression, I see a lot of information which I cannot quite understand. What does %e or %t represent on the left side of each line?
7. What does an alias operator do? It seems to have a significant role but I am not sure what that is.
8. It will be a good idea to have a documentation for each node_operator. Are they all associated with an activity? 
9. I have not understood the role of hash in sorting.
10. If we have a documentation of all operators in node_operator, it will also be good to associate each of them with the proper url of the documentation here: http://hpccsystems.com/download/docs/ecl-language-reference/html
11. I think it would be a great source of understanding ECL if each of the operators explained in here (http://hpccsystems.com/download/docs/ecl-language-reference/html) also contains examples of how it behaves on a given dataset. For example, we have an example dataset on which we can do these operation, and what changes are made with execution of each of these operations. There should be examples for each use of the given operator.
12. I see that there a few terms which mean the same thing. For examle dataset and recordset and table (I guess?), field and column, operation and activity. If possible, it would be better address them as one unique term.
13. Many similar topics here (http://hpccsystems.com/download/docs/ecl-language-reference/html) point to the same url. For example: Apply and Apply function. I am not sure whether that serves any purpose.
14. It would probably be a good idea to generate an error if an option is not available but used in command line. For example if I use something like -fanyrandomthing, it does not generate an error.

There are certain things which I believe were really valuable experiences for me during the internship. Getting to know how to work with github was a great addition to my resume. 
