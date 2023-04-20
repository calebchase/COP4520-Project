#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <functional>
#include <map>
#include <queue>
using namespace std;

struct HuffmanNode
{
    int val;
    int freq;
    HuffmanNode* l;
    HuffmanNode* r;

    HuffmanNode(int val, int freq)
    {
        this->val = val;
        this->freq = freq;
        l = r = nullptr;
    }

    HuffmanNode(pair<int, int> p)
    {
        val = p.first;
        freq = p.second;
        l = r = nullptr;
    }
};

struct CompareNodes 
{
    bool operator()(const HuffmanNode* lhs, const HuffmanNode* rhs) const
    {
        return lhs->freq > rhs->freq;
    }
};

HuffmanNode* buildTree(vector<pair<int, int>> table)
{
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, CompareNodes> pq;

    for(pair<int, int> p : table)
    {
        pq.push(new HuffmanNode(p));
    }

    while(pq.size() > 1)
    {
        HuffmanNode* l = pq.top();
        pq.pop();
        HuffmanNode* r = pq.top();
        pq.pop();
        HuffmanNode* p = new HuffmanNode(-1 , l->freq + r->freq);
        p->l = l;
        p->r = r;
        pq.push(p);
    }

    return pq.top();
}

map<int, string> buildTable(HuffmanNode* root)
{
    map<int, string> table;

    function<void(HuffmanNode*, string)> traverse = [&](HuffmanNode* node, string code)
    {
        if(node->l == nullptr && node->r == nullptr)
            table[node->val] = code;
        else
        {
            traverse(node->l, code + "0");
            traverse(node->r, code + "1");
        }
    };

    traverse(root, "");

    return table;
}

string encode(vector<int> vals, map<int, string> table)
{
    string str = "";
    
    for(int i : vals)
    {
        str += table[i];
    }

    return str;
}

vector<int> decode(string input, HuffmanNode* root)
{
    vector<int> result;

    HuffmanNode* curr = root;
    for(int i = 0; i < input.size(); i++)
    {
        if(input[i] == '0')
            curr = curr->l;
        else
            curr = curr->r;

        if(curr->l == nullptr && curr->r == nullptr)
        {
            result.push_back(curr->val);
            curr = root;
        }
    }

    return result;
}