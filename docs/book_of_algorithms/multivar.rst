======================
Sparse representations
======================

The sparse tree data structure
------------------------------

Yacas has a sparse tree object for use as a storage for storing
(key,value) pairs for which the following properties hold:

* ``(key, value1) + (key, value2) = (key, value1+value2)``
* ``(key1, value1) * (key2, value2) = (key1+key2, value1*value2)``

The last is optional. For multivariate polynomials (described
elsewhere) both hold, but for matrices, only the addition property
holds.  The function {MultiplyAddSparseTrees} (described below) should
not be used in these cases.

Internal structure
------------------

A key is defined to be a list of integer numbers ($ n[1] $, ..., $
n[m] $).  Thus for a two-dimensional key, one item in the sparse tree
database could be reflected as the (key,value) pair { {{1,2},3} },
which states that element {(1,2)} has value {3}. (Note: this is not
the way it is stored in the database!).

The storage is recursive. The sparse tree begins with a list of
objects { {n1,tree1} } for values of {n1} for the first item in the
key. The {tree1} part then contains a sub-tree for all the items in
the database for which the value of the first item in the key is {n1}.

The above single element could be created with::

  In> r:=CreateSparseTree({1,2},3)
  Out> {{1,{{2,3}}}};

{CreateSparseTree} makes a database with exactly one item.  Items can
now be obtained from the sparse tree with {SparseTreeGet}.::

  In> SparseTreeGet({1,2},r)
  Out> 3;
  In> SparseTreeGet({1,3},r)
  Out> 0;

And values can also be set or changed::

  In> SparseTreeSet({1,2},r,Current+5)
  Out> 8;
  In> r
  Out> {{1,{{2,8}}}};
  In> SparseTreeSet({1,3},r,Current+5)
  Out> 5;
  In> r
  Out> {{1,{{3,5},{2,8}}}};

The variable {Current} represents the current value, and can be used
to determine the new value. {SparseTreeSet} destructively modifies the
original, and returns the new value. If the key pair was not found, it
is added to the tree.

The sparse tree can be traversed, one element at a time, with
{SparseTreeScan}::

  In> SparseTreeScan(Hold({{k,v},Echo({k,v})}),2,r)
  {1,3} 5 
  {1,2} 8 

An example of the use of this function could be multiplying a sparse
matrix with a sparse vector, where the entire matrix can be scanned
with {SparseTreeScan}, and each non-zero matrix element $ A[i][j] $
can then be multiplied with a vector element $ v[j] $, and the result
added to a sparse vector $ w[i] $, using the {SparseTreeGet} and
{SparseTreeSet} functions.  Multiplying two sparse matrices would
require two nested calls to {SparseTreeScan} to multiply every item
from one matrix with an element from the other, and add it to the
appropriate element in the resulting sparse matrix.

When the matrix elements $ A[i][j] $ are defined by a function $
f(i,j) $ (which can be considered a dense representation), and it
needs to be multiplied with a sparse vector $ v[j] $, it is better to
iterate over the sparse vector $ v[j] $.  Representation defines the
most efficient algorithm to use in this case.

The API to sparse trees is:

* {CreateSparseTree(coefs,fact)} - Create a sparse tree with one
  monomial, where 'coefs' is the key, and 'fact' the value. 'coefs'
  should be a list of integers.
* {SparseTreeMap(op,depth,tree)} - Walk over the sparse tree, one
  element at a time, and apply the function "op" on the arguments
  (key,value). The 'value' in the tree is replaced by the value
  returned by the {op} function. 'depth' signifies the dimension of
  the tree (number of indices in the key).
* {SparseTreeScan(op,depth,tree)} - Same as SparseTreeMap, but without
  changing elements.
* {AddSparseTrees(depth,x,y)},
  {MultiplyAddSparseTrees(depth,x,y,coefs,fact)} - Add sparse tree 'y'
  to sparse tree 'x', destructively.  in the {MultiplyAdd} case, the
  monomials are treated as if they were multiplied by a monomial with
  coefficients with the (key,value) pair (coefs,fact). 'depth'
  signifies the dimension of the tree (number of indices in the key).
* {SparseTreeGet(key,tree)} - return value stored for key in the tree.
* {SparseTreeSet(key,tree,newvalue)} - change the value stored for the
  key to newvalue. If the key was not found then {newvalue} is stored
  as a new item. The variable {Current} is set to the old value (or
  zero if the key didn't exist in the tree) before evaluating
  {newvalue}.


Implementation of multivariate polynomials
------------------------------------------

This section describes the implementation of multivariate
polynomials in yacas.

Concepts and ideas are taken from the books [Davenport1988]_ and [Gathen1999]_.


Definitions
^^^^^^^^^^^

The following definitions define multivariate polynomials, and the
functions defined on them that are of interest for using such
multivariates.

A *term* is an object which can be written as 

.. math:: cx_1^{n_1}x_2^{n_2}\ldots x_m^{n_m}

for :math:`m` variables (:math:`x_1`, ..., :math:`x_m`). The numbers
:math:`n_m` are integers. :math:`c` is called a *coefficient*, and 
:math:`x_1^{n_1}x_2^{n_2}\ldots x_m^{n_m}` a *monomial*.

A *multivariate polynomial* is taken to be a sum over terms.

We write :math:`c_ax^a` for a term, where :math:`a` is a list of
powers for the monomial, and :math:`c_a` the *coefficient* of the 
term.

It is useful to define an ordering of monomials, to be able to determine the
canonical form of a multivariate. For the currently implemented code the
*lexicographic order* has been chosen:

* First, the ordering of variables is chosen, (:math:`x_1`, ..., :math:`x_m`)
* For the exponents of a monomial, :math:`a = (a_1,\ldots, a_m)`
  the lexicographic order first looks at the first exponent, :math:`a_1`,
  to determine which of the two monomials comes first in the
  multivariate.  If the two exponents are the same, the next exponent
  is considered.

This method is called *lexicographic* because it is similar to
the way words are ordered in a usual dictionary.

For all algorithms (including division) there is some freedom in the
ordering of monomials. One interesting advantage of the lexicographic
order is that it can be implemented with a recursive data structure,
where the first variable, :math:`x_1` can be treated as the main
variable, thus presenting it as a univariate polynomial in :math:`x_1`
with all its terms grouped together.

Other orderings can be used, by re-implementing a part of the code
dealing with multivariate polynomials, and then selecting the new code
to be used as a driver, as will be described later on.

Given the above ordering, the following definitions can be stated:

For a non-zero *multivariate polynomial*

.. math:: f = \sum_{a=a_{max}}^{a_{min}}c_ax^a

with a monomial order:

#. :math:`c_ax^a` is a *term* of the multivariate.
#. the *multidegree* of :math:`f` is :math:`\operatorname{mdeg}(f) := a_{max}`.
#. the *leading coefficient* of :math:`f` is :math:`\operatorname{lc}(f):=c_{\operatorname{mdeg}(f)}`, for the first term with non-zero coefficient.
#. the *leading monomial* of :math:`f` is :math:`\operatorname{lm}(f):=x^{\operatorname{mdeg}(f)}`.
#. the *leading term* of :math:`f` is :math:`\operatorname{lt}(f):=\operatorname{lc}(f)\operatorname{lm}(f)`.

The above define access to the leading monomial, which is used for
divisions, gcd calculations and the like. Thus an implementation needs
be able to determine :math:`(\operatorname{mdeg}(f),\operatorname{lc}(f)`.
Note the similarity with the (key,value) pairs described in the sparse tree
section. :math:`\operatorname{mdeg}(f)` can be thought of as a key, and 
:math:`\operatorname{lc}(f)` as a value.

The *multicontent*, :math:`\operatorname{multicont}(f)`, is defined to be a
term that divides all the terms in :math:`f`, and is the term described by
:math:`(\min(a), \gcd(c))`, with :math:`\gcd(c)` the GCD of all the
coefficients, and :math:\min(a)` the lowest exponents for each variable,
occurring in :math:`f` for which :math:`c` is non-zero.

The *multiprimitive part* is then defined as 
:math:`\operatorname{pp}(f):=\frac{f}{\operatorname{multicont}(f)}`.

For a multivariate polynomial, the obvious addition and (distributive)
multiplication rules hold

* :math:`(a+b) + c = a+(b+c)`
* :math:`a(b+c) = ab+ac`

These are supported in the Yacas system through a multiply-add
operation: :math:`\operatorname{muadd}(f,t,g) := f+tg`.  This allows for both
adding two polynomials (:math:`t=1`), or multiplication of two polynomials by
scanning one polynomial, and multiplying each term of the scanned
polynomial with the other polynomial, and adding the result to the
polynomial that will be returned. Thus there should be an efficient
:math:`\operatorname{muadd}` operation in the system.


Representation
^^^^^^^^^^^^^^

For the representation of polynomials, on computers it is natural to
do this in an array: :math:`(a_1, a_2,\ldots, a_n)` for a univariate
polynomial, and the equivalent for multivariates. This is called a
*dense* representation, because all the coefficients are stored,
even if they are zero.  Computers are efficient at dealing with
arrays. However, in the case of multivariate polynomials, arrays can
become rather large, requiring a lot of storage and processing power
even to add two such polynomials. For instance, :math:`x^{200}y^{100}z^{300}+1`
could take 6000000 places in an array for the coefficients. Of
course variables could be substituted for the single factors,
:math:`p:=x^{200}` *etc.*, but it requires an additional *ad hoc* step.

An alternative is to store only the terms for which the coefficients
are non-zero. This adds a little overhead to polynomials that could
efficiently be stored in a dense representation, but it is still
little memory, whereas large sparse polynomials are stored in
acceptable memory too. It is of importance to still be able to add,
multiply divide and get the leading term of a multivariate polynomial,
when the polynomial is stored in a sparse representation.

For the representation, the data structure containing the
{(exponents,coefficient)} pair can be viewed as a database holding
{(key,value)} pairs, where the list of exponents is the key, and the
coefficient of the term is the value stored for that key. Thus, for a
variable set {{x,y}} the list ``{{1,2},3}`` represents :math:`3xy^2`.

Yacas stores multivariates internally as ``MultiNomial(vars, terms)``,
where ``vars`` is the ordered list of variables, and terms some object
storing all the ``(key, value)`` pairs representing the terms.  Note we
keep the storage vague: the ``terms`` placeholder is implemented by
other code, as a database of terms. The specific representation can be
configured at startup (this is described in more detail below).

For the current version, yacas uses the sparse tree representation,
which is a recursive sparse representation.  For example, for a
variable set ``{x,y,z}``, the ``terms`` object contains a list of objects
of form ``{deg,terms}``, one for each degree ``deg`` for the variable ``x``
occurring in the polynomial. The ``terms`` part of this object is then a
sub-sparse tree for the variables ``{y,z}``.

An explicit example::

  In> MM(3*x^2+y)
  Out> MultiNomial({x,y},{{2,{{0,3}}},{0,{{1,1}, {0,0}}}});

The first item in the main list is ``{2,{{0,3}}}``, which states that
there is a term of the form :math:`x^2y^03`. The second item states that
there are two terms, :math:`x^0y^11` and :math:`x^0y^00 = 0`.

This representation is sparse::

  In> r:=MM(x^1000+x)
  Out> MultiNomial({x},{{1000,1},{1,1}});

and allows for easy multiplication::

  In> r*r
  Out> MultiNomial({x},{{2000,1},{1001,2},{2,1},{0,0}});
  In> NormalForm(%)
  Out> x^2000+2*x^1001+x^2;

Internal code organization
^^^^^^^^^^^^^^^^^^^^^^^^^^

The implementation of multivariates can be divided in three levels.

At the top level are the routines callable by the user or the rest of
the system: :func:`MultiDegree`, :func:`MultiDivide`, :func:`MultiGcd`,
:func:`Groebner`, *etc.*  In general, this is the level implementing the
operations actually desired.

The middle level does the book-keeping of the ``MultiNomial(vars,terms)``
expressions, using the functionality offered by the lowest level.

For the current system, the middle level is in ``multivar.rep/sparsenomial.ys``,
and it uses the sparse tree representation implemented in ``sparsetree.ys``.

The middle level is called the *driver*, and can be changed, or
re-implemented if necessary. For instance, in case calculations need
to be done for which dense representations are actually acceptable,
one could write C++ implementing above-mentioned database structure,
and then write a middle-level driver using the code.  The driver can
then be selected at startup. In the file ``yacasinit.ys`` the default
driver is chosen, but this can be overridden in the ``.yacasrc`` file or
some file that is loaded, or at the command line, as long as it is
done before the multivariates module is loaded (which loads the
selected driver). Driver selection is as simple as setting a global
variable to contain a file name of the file implementing the driver::

    Set(MultiNomialDriver,
      "multivar.rep/sparsenomial.ys");

where "multivar.rep/sparsenomial.ys" is the file implementing the
driver (this is also the default driver, so the above command would
not change any thing).

The choice was made for static configuration of the driver before the
system starts up because it is expected that there will in general be
one best way of doing it, given a certain system with a certain set of
libraries installed on the operating system, and for a specific
version of Yacas. The best version can then be selected at start up,
as a configuration step. The advantage of static selection is that no
overhead is imposed: there is no performance penalty for the
abstraction layers between the three levels.

Driver interface
^^^^^^^^^^^^^^^^

The driver should implement the following interface:

* ``CreateTerm(vars,{exp,coef})`` - create a multivariate polynomial
  with one term, in the variables defined in ``var``, with the
  (key,value) pair (coefs,fact)
* ``MultiNomialAdd(multi1, multi2)`` - add two multivars, and
  (possibly) destructively modify ``multi1`` to contain the result::
      
    [ multi1 := multi1 + multi2; multi1; ];
    
* ``MultiNomialMultiplyAdd(multi1, multi2,exp,coef)`` - add two
  multivars, and (possibly) destructively modify ``multi1`` to contain the
  result. ``multi2`` is considered multiplied by a term represented by the
  (key,value) pair (exp,coef)::
  
    [ multi1 := multi1 + term * multi2; multi1; ];
    
* ``MultiNomialNegate(multi)`` - negate a multivar, returning -multi,
  and destructively changing the original::
  
    [ multi := - multi; multi1; ];
  
* ``MultiNomialMultiply(multi1,multi2)`` - Multiply two multivars, and
  (possibly) destructively modify ``multi1`` to contain the result,
  returning the result::
  
    [ multi1 := multi1 * multi2; multi1; ];
    
* ``NormalForm(multi)`` - convert MultiNomial to normal form (as would
  be typed in be the user).  This is part of the driver because the
  driver might be able to do this more efficiently than code above it
  which can use :func:`ScanMultiNomial`.
* ``MultiLeadingTerm(multi)`` - return the (key,value) pair
  (mdeg(f),lc(f)) representing the leading term. This is all the
  information needed about the leading term, and thus the leading
  coefficient and multidegree can be extracted from it.
* ``MultiDropLeadingZeroes(multi)`` - remove leading terms with zero
  factors.
* ``MultiTermLess(x,y)`` - for two (key,value) pairs, return :data:`True` if
  :math:`x<y`, where the operation :math:`<` is the one used for the
  representation, and :data:`False` otherwise.
* ``ScanMultiNomial(op,multi)`` - traverse all the terms of the
  multivariate, applying the function ``op`` to each (key,value) pair
  (exp,coef). The monomials are traversed in the ordering defined by
  MultiTermLess. ``op`` should be a function accepting two arguments.
* ``MultiZero(multi)`` - return :data:`True` if the multivariate is zero (all
  coefficients are zero), :data:`False` otherwise.
