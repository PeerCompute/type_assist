#include "NGramTrie.h"

#include <algorithm>

using namespace std;


// ------------------------------------
// INSERT CORPUS SENTENCE
// ------------------------------------

void NGramTrie::insertSentence(
    const vector<string>& words,
    WordTable& wordTable
)
{
    int n = static_cast<int>(words.size());

    for (int i = 0; i < n; i++)
    {
        NGramNode* current = &root;

        for (int j = i; j < n && j < i + MAX_DEPTH; j++)
        {
            int id = wordTable.addWord(words[j]);

            if (current->children.find(id) == current->children.end())
            {
                current->children[id] = make_unique<NGramNode>();
            }

            current = current->children[id].get();

            current->frequency++;
        }
    }
}


// ------------------------------------
// ADD SENTENCE
// ------------------------------------

void NGramTrie::addSentence(
    const vector<string>& words,
    WordTable& wordTable
)
{
    if (!words.empty())
        insertSentence(words, wordTable);
}


// ------------------------------------
// BUILD TOP K CACHE
// ------------------------------------

void NGramTrie::refreshNode(
    NGramNode* node,
    const WordTable& wordTable
)
{
    node->topK.clear();

    vector<pair<int, int>> candidates;

    for (const auto& child : node->children)
    {
        int wordID = child.first;

        int totalFrequency =
    child.second->frequency +
    USER_WEIGHT * child.second->userFrequency;

        candidates.push_back({
            wordID,
            totalFrequency
        });
    }

    sort(
        candidates.begin(),
        candidates.end(),
        [&](const auto& a, const auto& b)
        {
            if (a.second != b.second)
                return a.second > b.second;

            return wordTable.getWord(a.first) <
                   wordTable.getWord(b.first);
        }
    );

    if (candidates.size() > K)
        candidates.resize(K);

    node->topK = candidates;
}

void NGramTrie::buildCache(
    NGramNode* node,
    const WordTable& wordTable
)
{
    refreshNode(node, wordTable);

    for (auto& child : node->children)
    {
        buildCache(child.second.get(), wordTable);
    }
}


// ------------------------------------
// BUILD TOP K FOR ENTIRE TRIE
// ------------------------------------

void NGramTrie::buildTopK(const WordTable& wordTable)
{
    buildCache(&root, wordTable);
}


// ------------------------------------
// GET CANDIDATES USING PREFIX
// ------------------------------------

vector<pair<int, int>> NGramTrie::getCandidates(
    const NGramNode* node,
    const string& prefix,
    const WordTable& wordTable,
    int k
) const
{
    vector<pair<int, int>> candidates;

    if (node == nullptr)
        return candidates;

    // If there is no prefix, use cached Top K.
    if (prefix.empty())
    {
        candidates = node->topK;
    }
    else
    {
        // Search all children matching the prefix.
        for (const auto& child : node->children)
        {
            int wordID = child.first;

            string word = wordTable.getWord(wordID);

            if (word.compare(0, prefix.size(), prefix) == 0)
            {
                int totalFrequency =
                    child.second->frequency +
                    child.second->userFrequency;

                candidates.push_back({
                    wordID,
                    totalFrequency
                });
            }
        }

        sort(
            candidates.begin(),
            candidates.end(),
            [&](const auto& a, const auto& b)
            {
                if (a.second != b.second)
                    return a.second > b.second;

                return wordTable.getWord(a.first) <
                       wordTable.getWord(b.first);
            }
        );
    }

    if (static_cast<int>(candidates.size()) > k)
        candidates.resize(k);

    return candidates;
}


// ------------------------------------
// LEARN FROM USER INPUT
// ------------------------------------

void NGramTrie::observeWord(
    const vector<string>& previousWords,
    const string& word,
    WordTable& wordTable
)
{
    int wordID = wordTable.addWord(word);

    // We support up to two previous words.
    int maxContext = min(2, static_cast<int>(previousWords.size()));

    // Update unigram, bigram and trigram counts.
    for (int contextLength = 0;
         contextLength <= maxContext;
         contextLength++)
    {
        NGramNode* current = &root;

        int start =
            static_cast<int>(previousWords.size()) - contextLength;

        bool validContext = true;

        // Traverse the previous-word context.
        for (int i = start;
             i < static_cast<int>(previousWords.size());
             i++)
        {
            int id = wordTable.getId(previousWords[i]);

            if (id == -1)
            {
                validContext = false;
                break;
            }

            if (current->children.find(id) == current->children.end())
            {
                current->children[id] = make_unique<NGramNode>();
            }

            current = current->children[id].get();
        }

        if (!validContext)
            continue;

        // Add the completed word.
        if (current->children.find(wordID) == current->children.end())
        {
            current->children[wordID] = make_unique<NGramNode>();
        }

        current->children[wordID]->userFrequency++;

        // Only this context's Top K changed.
        refreshNode(current, wordTable);
    }
}


// ------------------------------------
// PREDICTION
// ------------------------------------

vector<pair<string, int>> NGramTrie::predict(
    const vector<string>& previousWords,
    const string& prefix,
    const WordTable& wordTable,
    int k
) const
{
    vector<pair<string, int>> results;

    if (k <= 0)
        return results;

    vector<const NGramNode*> contexts;

    // --------------------------------
    // 1. TRIGRAM CONTEXT
    // --------------------------------

    if (previousWords.size() >= 2)
    {
        int firstID =
            wordTable.getId(previousWords[previousWords.size() - 2]);

        int secondID =
            wordTable.getId(previousWords.back());

        if (firstID != -1 && secondID != -1)
        {
            auto first = root.children.find(firstID);

            if (first != root.children.end())
            {
                auto second =
                    first->second->children.find(secondID);

                if (second != first->second->children.end())
                {
                    contexts.push_back(second->second.get());
                }
            }
        }
    }

    // --------------------------------
    // 2. BIGRAM CONTEXT
    // --------------------------------

    if (!previousWords.empty())
    {
        int id = wordTable.getId(previousWords.back());

        if (id != -1)
        {
            auto it = root.children.find(id);

            if (it != root.children.end())
            {
                contexts.push_back(it->second.get());
            }
        }
    }

    // --------------------------------
    // 3. UNIGRAM CONTEXT
    // --------------------------------

    contexts.push_back(&root);


    // --------------------------------
    // TRY CONTEXTS IN ORDER
    // --------------------------------

    for (const NGramNode* context : contexts)
    {
        vector<pair<int, int>> candidates =
            getCandidates(
                context,
                prefix,
                wordTable,
                k
            );

        if (!candidates.empty())
        {
            for (const auto& candidate : candidates)
            {
                results.push_back({
                    wordTable.getWord(candidate.first),
                    candidate.second
                });
            }

            return results;
        }
    }

    return results;
}