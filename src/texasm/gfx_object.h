#pragma once


#include "texasm.h"
#include <list>


class CGfxObject {
public:
    virtual ~CGfxObject() = default;

    CGfxObject(char* _name, const int _resgroup): x(0), y(0) {
        name = _name;
        placed = false;
        resgroup = _resgroup;
    }

    char* GetName() const {
        return name;
    }

    void Place(const int _x, const int _y) {
        x = _x;
        y = _y;
        placed = true;
    }

    bool IsPlaced() const {
        return placed;
    }

    int GetResGroup() const {
        return resgroup;
    }

    void GetTargetPos(int* _x, int* _y) const {
        *_x = x;
        *_y = y;
    }

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual HTEXTURE GetTexture() = 0;
    virtual void GetSourcePos(int* _x, int* _y) = 0;
    virtual bool SaveDescription(FILE* fp, char* texname) = 0;

protected:
    char* name;
    int resgroup;
    bool placed;
    int x;
    int y;
};


// sorting predicate
#include <functional>

struct ByLargestDimensionDescending : public std::greater<CGfxObject *> {
    bool operator()(CGfxObject* & a, CGfxObject* & b) const {
        return (a->GetWidth() < b->GetWidth() && a->GetWidth() > b->GetHeight()) ||
                (a->GetHeight() > b->GetWidth() && a->GetHeight() > b->GetHeight());
    }
};


// list and iterator

typedef std::list<CGfxObject *> GfxObjList;
typedef std::list<CGfxObject *>::iterator GfxObjIterator;
