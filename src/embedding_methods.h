//
// Created by Adoni1203 on 16/7/20.
//

#ifndef DLNE_EMBEDDING_METHODS_H
#define DLNE_EMBEDDING_METHODS_H

#include "graph_data.h"
#include "cnn/cnn.h"
#include "cnn/expr.h"
#include "cnn/lstm.h"
#include "cnn/gru.h"
#include "cnn/dict.h"
#include <vector>
#include <string>

using namespace cnn;

class ContentEmbeddingMethod {
public:
    virtual ~ContentEmbeddingMethod() { }

    virtual Expression get_embedding(const CONTENT_TYPE &content, const TFIDF_TYPE &tfidf, ComputationGraph &cg) = 0;

    std::string get_method_name() {
        return method_name;
    }
    void initial_look_up_table(std::string file_name, Dict &d) {
        std::cout << "Initializing lookup table from " << file_name << " ..." <<std::endl;
        std::string UNK="UNKNOWN_WORD";
        std::ifstream em_in(file_name);
        assert(em_in);
        unsigned em_count, em_size;
        int unknow_id = d.Convert(UNK);
        em_in >> em_count >> em_size;
        assert(em_size==W_EM_DIM);
        std::vector<float> e(em_size);
        std::string w;
        int initialized_word_count=0;
        for (int i = 0; i < em_count; i++) {
            em_in >> w;
            for (int j = 0; j < em_size; j++) {
                em_in >> e[j];
            }
            int index = d.Convert(w);
            if (index == unknow_id) continue;
            initialized_word_count++;
            assert(index<d.size() && index>=0);
            p->Initialize(index, e);
        }
        std::cout << "Initialize " << initialized_word_count << " words" << std::endl;
        std::cout << d.size() - initialized_word_count << " words not initialized" << std::endl;
    }
    LookupParameters *p;
    unsigned W_EM_DIM;
    unsigned C_EM_DIM;
    std::string method_name;
};

class WordAvg_CE : public ContentEmbeddingMethod {
public:
    explicit WordAvg_CE(
            Model& model,
            unsigned word_embedding_size,
            unsigned content_embedding_size,
            std::string word_embedding_file,
            Dict &d){
        assert(word_embedding_size==content_embedding_size);
        this->W_EM_DIM = word_embedding_size;
        this->C_EM_DIM = content_embedding_size;
        this->method_name = "WordAvg";
        p = model.add_lookup_parameters(d.size(), {W_EM_DIM});
        initial_look_up_table(word_embedding_file, d);
    }

    ~WordAvg_CE() {}

    Expression get_embedding(const CONTENT_TYPE &content, const TFIDF_TYPE &tfidf, ComputationGraph &cg){
        std::vector<Expression> all_word_embedding;
        for (int i=0;i<content.size();i++){
            std::vector<Expression> sentence_expression;
            for (int j=0;j<content[i].size();j++){
                sentence_expression.push_back(tfidf[i][j]*lookup(cg, p, content[i][j]));
            }
            all_word_embedding.push_back(average(sentence_expression));
        }
        return average(all_word_embedding);
    }
};

class GRU_CE : public ContentEmbeddingMethod {
public:
    GRUBuilder builder;
    explicit GRU_CE(
            Model& model,
            unsigned word_embedding_size,
            unsigned content_embedding_size,
            std::string word_embedding_file,
            Dict &d){
        this->W_EM_DIM = word_embedding_size;
        this->C_EM_DIM = content_embedding_size;
        this->method_name = "GRU";
        builder=GRUBuilder(1, W_EM_DIM, C_EM_DIM, &model);
        p = model.add_lookup_parameters(d.size(), {W_EM_DIM});
        initial_look_up_table(word_embedding_file, d);
    }

    ~GRU_CE() {}

    Expression get_embedding(const CONTENT_TYPE &content, const TFIDF_TYPE &tfidf, ComputationGraph &cg){
        std::vector<Expression> all_hidden;

        for (auto c:content){
            builder.new_graph(cg);
            builder.start_new_sequence();
            std::vector<Expression> sent;
            for(auto w:c){
                Expression i_x_t = lookup(cg, p, w);
                sent.push_back(builder.add_input(i_x_t));
            }
            all_hidden.push_back(average(sent));
        }
        return average(all_hidden);
    }
};

#endif //DLNE_EMBEDDING_METHODS_H
