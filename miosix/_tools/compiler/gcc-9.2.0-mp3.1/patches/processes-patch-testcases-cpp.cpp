
class Base
{
public:
    virtual ~Base() {}
};

Base *mkbase()
{
   return new Base;
}
