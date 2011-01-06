
/* apply_to_each is similar to transform, except it doesn't store the output in another
 * container and it doesn't require the op to return a value. It is good to use with
 * member functions with the setter and getter pattern. (op = std::mem_fun_ref(&ObjType:Setter)
 */
template < class InputIterator, class InputIterator2, class BinaryOperator >
inline void apply_to_each ( InputIterator first1, InputIterator last1, InputIterator2 second1,
                              BinaryOperator op )
{
  while (first1 != last1)
    op(*first1++, *second1++);
}