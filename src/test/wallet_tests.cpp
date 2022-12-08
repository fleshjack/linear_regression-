#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"

// how many times to run all the tests to have a chance to catch errors that only show up with particular random shuffles
#define RUN_TESTS 100

// some tests fail 1% of the time due to bad luck.
// we repeat those tests this many times and only complain if all iterations of the test fail
#define RANDOM_REPEATS 5

using namespace std;

typedef set<pair<const CWalletTx*,unsigned int> > CoinSet;

BOOST_AUTO_TEST_SUITE(wallet_tests)

static CWallet wallet;
static vector<COutput> vCoins;

static void add_coin(int64 nValue, int nAge = 6*24, bool fIsFromMe = false, int nInput=0)
{
    static int i;
    CTransaction* tx = new CTransaction;
    tx->nLockTime = i++;        // so all transactions get different hashes
    tx->vout.resize(nInput+1);
    tx->vout[nInput].nValue = nValue;
    CWalletTx* wtx = new CWalletTx(&wallet, *tx);
    delete tx;
    if (fIsFromMe)
    {
        // IsFromMe() returns (GetDebit() > 0), and GetDebit() is 0 if vin.empty(),
        // so stop vin being empty, and cache a non-zero Debit to fake out IsFromMe()
        wtx->vin.resize(1);
        wtx->fDebitCached = true;
        wtx->nDebitCached = 1;
    }
    COutput output(wtx, nInput, nAge);
    vCoins.push_back(output);
}

static void empty_wallet(void)
{
    BOOST_FOREACH(COutput output, vCoins)
        delete output.tx;
    vCoins.clear();
}

static bool equal_sets(CoinSet a, CoinSet b)
{
    pair<CoinSet::iterator, CoinSet::iterator> ret = mismatch(a.begin(), a.end(), b.begin());
    return ret.first == a.end() && ret.second == b.end();
}

BOOST_AUTO_TEST_CASE(coin_selection_tests)
{
    static CoinSet setCoinsRet, setCoinsRet2;
    static int64 nValueRet;

    // test multiple times to allow for differences in the shuffle order
    for (int i = 0; i < RUN_TESTS; i++)
    {
        empty_wallet();

        // with an empty wallet we can't even pay one cent
        BOOST_CHECK(!wallet.SelectCoinsMinConf( 1 * CENT, 1, 6, vCoins, setCoinsRet, nValueRet));

        add_coin(1*CENT, 4);        // add a new 1 cent coin

        // with a new 1 cent coin, we still can't find a mature 1 cent
        BOOST_CHECK(!wallet.SelectCoinsMinConf( 1 * CENT, 1, 6, vCoins, setCoinsRet, nValueRet));

        // but we can find a new 1 cent
        BOOST_CHECK( wallet.SelectCoinsMinConf( 1 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 1 * CENT);

        add_coin(2*CENT);           // add a mature 2 cent coin

        // we can't make 3 cents of mature coins
        BOOST_CHECK(!wallet.SelectCoinsMinConf( 3 * CENT, 1, 6, vCoins, setCoinsRet, nValueRet));

        // we can make 3 cents of new  coins
        BOOST_CHECK( wallet.SelectCoinsMinConf( 3 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 3 * CENT);

        add_coin(5*CENT);           // add a mature 5 cent coin,
        add_coin(10*CENT, 3, true); // a new 10 cent coin sent from one of our own addresses
        add_coin(20*CENT);          // and a mature 20 cent coin

        // now we have new: 1+10=11 (of which 10 was self-sent), and mature: 2+5+20=27.  total = 38

        // we can't make 38 cents only if we disallow new coins:
        BOOST_CHECK(!wallet.SelectCoinsMinConf(38 * CENT, 1, 6, vCoins, setCoinsRet, nValueRet));
        // we can't even make 37 cents if we don't allow new coins even if they're from us
        BOOST_CHECK(!wallet.SelectCoinsMinConf(38 * CENT, 6, 6, vCoins, setCoinsRet, nValueRet));
        // but we can make 37 cents if we accept new coins from ourself
        BOOST_CHECK( wallet.SelectCoinsMinConf(37 * CENT, 1, 6, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 37 * CENT);
        // and we can make 38 cents if we accept all new coins
        BOOST_CHECK( wallet.SelectCoinsMinConf(38 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 38 * CENT);

        // try making 34 cents from 1,2,5,10,20 - we can't do it exactly
        BOOST_CHECK( wallet.SelectCoinsMinConf(34 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_GT(nValueRet, 34 * CENT);         // but should get more than 34 cents
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3);     // the best should be 20+10+5.  it's incredibly unlikely the 1 or 2 got included (but possible)

        // when we try making 7 cents, the smaller coins (1,2,5) are enough.  We should see just 2+5
        BOOST_CHECK( wallet.SelectCoinsMinConf( 7 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 7 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 2);

        // when we try making 8 cents, the smaller coins (1,2,5) are exactly enough.
        BOOST_CHECK( wallet.SelectCoinsMinConf( 8 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK(nValueRet == 8 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3);

        // when we try making 9 cents, no subset of smaller coins is enough, and we get the next bigger coin (10)
        BOOST_CHECK( wallet.SelectCoinsMinConf( 9 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 10 * CENT);
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1);

        // now clear out the wallet and start again to test choosing between subsets of smaller coins and the next biggest coin
        empty_wallet();

        add_coin( 6*CENT);
        add_coin( 7*CENT);
        add_coin( 8*CENT);
        add_coin(20*CENT);
        add_coin(30*CENT); // now we have 6+7+8+20+30 = 71 cents total

        // check that we have 71 and not 72
        BOOST_CHECK( wallet.SelectCoinsMinConf(71 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK(!wallet.SelectCoinsMinConf(72 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));

        // now try making 16 cents.  the best smaller coins can do is 6+7+8 = 21; not as good at the next biggest coin, 20
        BOOST_CHECK( wallet.SelectCoinsMinConf(16 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 20 * CENT); // we should get 20 in one coin
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 1);

        add_coin( 5*CENT); // now we have 5+6+7+8+20+30 = 75 cents total

        // now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, better than the next biggest coin, 20
        BOOST_CHECK( wallet.SelectCoinsMinConf(16 * CENT, 1, 1, vCoins, setCoinsRet, nValueRet));
        BOOST_CHECK_EQUAL(nValueRet, 18 * CENT); // we should get 18 in 3 coins
        BOOST_CHECK_EQUAL(setCoinsRet.size(), 3);

        add_coin( 18*CENT); // now we have 5+6+7+8+18+20+30

        // and now if we try making 16 cents again, the smaller coins can make 5+6+7 = 18 cents, the same as the next biggest coin, 18
  